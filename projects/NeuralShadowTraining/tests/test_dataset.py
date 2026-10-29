import struct
import tempfile
from pathlib import Path
import unittest
import zlib

from fdw_neural_shadows.dataset import COMPLETION_MARKER, SAMPLE_MAGIC, load_sample


class DatasetTests(unittest.TestCase):
    def test_sample_decoder(self) -> None:
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
        content_crc = zlib.crc32(payload[block_table_offset:]) & 0xFFFFFFFF
        struct.pack_into("<IIIIQQQIIIIQIIII", payload, 0, SAMPLE_MAGIC, 1, header_size, record_size,
                         file_size, 4, 9, 1, 0, 80, block_table_offset, payload_offset,
                         0, content_crc, COMPLETION_MARKER, 0)

        header = bytearray(payload[:header_size])
        struct.pack_into("<I", header, 64, 0)
        struct.pack_into("<I", payload, 64, zlib.crc32(header) & 0xFFFFFFFF)

        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "sample.nssample"
            path.write_bytes(payload)
            sample = load_sample(path)

        self.assertEqual(sample.sample_id, 4)
        self.assertEqual(sample.generation_id, 9)
        self.assertEqual(sample.blocks[6].data, block_data)


if __name__ == "__main__":
    unittest.main()
