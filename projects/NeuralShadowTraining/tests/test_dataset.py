import struct
import tempfile
from pathlib import Path
import unittest
import zlib

from fdw_neural_shadows.dataset import COMPLETION_MARKER, SAMPLE_MAGIC, DatasetFormatError, Sample, load_sample


class DatasetTests(unittest.TestCase):
    @staticmethod
    def make_payload() -> bytearray:
        header_size = 224
        record_size = 40
        block_table_offset = header_size
        payload_offset = 272

        block_data = b"physical-target"

        file_size = 288

        payload = bytearray(file_size)
        struct.pack_into("<IIQQQII", payload, block_table_offset, 6, 1, payload_offset,
                         len(block_data), 1, zlib.crc32(block_data) & 0xFFFFFFFF, 0)

        payload[payload_offset:payload_offset + len(block_data)] = block_data
        struct.pack_into("<IIIIQQQIIIIQIIII", payload, 0, SAMPLE_MAGIC, 1, header_size, record_size,
                         file_size, 4, 9, 1, 0, 80, block_table_offset, payload_offset,
                         0, 0, COMPLETION_MARKER, 0)

        DatasetTests.update_checksums(payload)
        return payload

    @staticmethod
    def update_checksums(payload: bytearray) -> None:
        struct.pack_into("<I", payload, 68, zlib.crc32(payload[224:]) & 0xFFFFFFFF)
        struct.pack_into("<I", payload, 64, 0)
        struct.pack_into("<I", payload, 64, zlib.crc32(payload[:224]) & 0xFFFFFFFF)

    @staticmethod
    def decode(payload: bytearray) -> Sample:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "sample.nssample"
            path.write_bytes(payload)
            return load_sample(path)

    def test_sample_decoder(self) -> None:
        sample = self.decode(self.make_payload())

        self.assertEqual(sample.sample_id, 4)
        self.assertEqual(sample.generation_id, 9)
        self.assertEqual(sample.blocks[6].data, b"physical-target")

    def test_header_validation_precedes_checksums(self) -> None:
        for offset, value, message in (
            (0, 0, "sample identity/version mismatch"),
            (4, 2, "sample identity/version mismatch"),
            (12, 0, "block table layout is unsupported"),
            (40, 33, "block table layout is unsupported"),
            (72, 0, "sample is incomplete or has unsupported flags"),
            (76, 1, "sample is incomplete or has unsupported flags"),
        ):
            with self.subTest(offset=offset):
                payload = self.make_payload()
                struct.pack_into("<I", payload, offset, value)
                with self.assertRaisesRegex(DatasetFormatError, f"^{message}$"):
                    self.decode(payload)

    def test_checksum_diagnostics(self) -> None:
        for offset, message in (
            (80, "header checksum mismatch"),
            (272, "content checksum mismatch"),
        ):
            with self.subTest(offset=offset):
                payload = self.make_payload()
                payload[offset] ^= 1
                with self.assertRaisesRegex(DatasetFormatError, f"^{message}$"):
                    self.decode(payload)

        payload = self.make_payload()
        payload[272] ^= 1
        self.update_checksums(payload)
        with self.assertRaisesRegex(DatasetFormatError, "^sample block checksum mismatch$"):
            self.decode(payload)

    def test_block_bounds_and_identity(self) -> None:
        for offset, encoding, value in (
            (224, "<I", 0),
            (228, "<I", 0),
            (232, "<Q", 271),
            (240, "<Q", 1000),
            (260, "<I", 1),
        ):
            with self.subTest(offset=offset):
                payload = self.make_payload()
                struct.pack_into(encoding, payload, offset, value)
                self.update_checksums(payload)
                with self.assertRaisesRegex(DatasetFormatError, "^sample block is invalid or out of bounds$"):
                    self.decode(payload)

    @staticmethod
    def make_two_block_payload() -> bytearray:
        payload = DatasetTests.make_payload()
        payload.extend(bytes(48))
        struct.pack_into("<Q", payload, 16, len(payload))
        struct.pack_into("<I", payload, 40, 2)
        struct.pack_into("<Q", payload, 56, 304)
        for index, (kind, start, data) in enumerate(((6, 304, b"physical-target"), (7, 320, b"second"))):
            struct.pack_into("<IIQQQII", payload, 224 + index * 40, kind, 1, start,
                             len(data), 1, zlib.crc32(data) & 0xFFFFFFFF, 0)
            payload[start:start + len(data)] = data
        DatasetTests.update_checksums(payload)
        return payload

    def test_block_overlap_and_duplicate_diagnostics(self) -> None:
        self.assertEqual(self.decode(self.make_two_block_payload()).blocks[7].data, b"second")
        for offset, encoding, value in ((264, "<I", 6), (272, "<Q", 304)):
            with self.subTest(offset=offset):
                payload = self.make_two_block_payload()
                struct.pack_into(encoding, payload, offset, value)
                self.update_checksums(payload)
                with self.assertRaisesRegex(DatasetFormatError, "^sample blocks overlap or repeat a kind$"):
                    self.decode(payload)

    def test_nonoverlapping_records_need_not_be_sorted(self) -> None:
        payload = self.make_two_block_payload()
        payload[224:264], payload[264:304] = payload[264:304], payload[224:264]
        self.update_checksums(payload)
        sample = self.decode(payload)
        self.assertEqual(sample.blocks[6].data, b"physical-target")
        self.assertEqual(sample.blocks[7].data, b"second")


if __name__ == "__main__":
    unittest.main()
