"""Bounded decoder for the versioned neural-shadow sample container."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import struct
import zlib

SAMPLE_MAGIC = 0x3153534E
SAMPLE_VERSION = 1
COMPLETION_MARKER = 0x454E4F44
MAX_FILE_BYTES = 2 * 1024 * 1024 * 1024
MAX_BLOCKS = 32

_HEADER = struct.Struct("<IIIIQQQIIIIQIIII")
_BLOCK = struct.Struct("<IIQQQII")

class DatasetFormatError(ValueError):
    """Raised when a sample fails a frozen container invariant."""

@dataclass(frozen=True)
class Block:
    kind: int
    version: int
    element_count: int
    data: bytes


@dataclass(frozen=True)
class Sample:
    sample_id: int
    generation_id: int
    blocks: dict[int, Block]


def load_sample(path: str | Path) -> Sample:
    """Load and validate one little-endian sample without trusting offsets."""

    payload = Path(path).read_bytes()
    if len(payload) < _HEADER.size or len(payload) > MAX_FILE_BYTES:
        raise DatasetFormatError("sample size is outside the supported range")

    values = _HEADER.unpack_from(payload)
    (
        magic, version, header_bytes, block_record_bytes, file_bytes,
        sample_id, generation_id, block_count, padding0, layout_offset,
        block_table_offset, payload_offset, header_crc, content_crc,
        completion, flags,
    ) = values

    if magic != SAMPLE_MAGIC or version != SAMPLE_VERSION:
        raise DatasetFormatError("sample identity/version mismatch")

    if file_bytes != len(payload) or header_bytes < _HEADER.size:
        raise DatasetFormatError("declared sample size is inconsistent")

    if block_record_bytes != _BLOCK.size or block_count > MAX_BLOCKS:
        raise DatasetFormatError("block table layout is unsupported")

    if padding0 or flags or completion != COMPLETION_MARKER:
        raise DatasetFormatError("sample is incomplete or has unsupported flags")

    if layout_offset < _HEADER.size or block_table_offset < header_bytes:
        raise DatasetFormatError("header offsets are inconsistent")

    table_end = block_table_offset + block_count * block_record_bytes
    if table_end > payload_offset or payload_offset > len(payload):
        raise DatasetFormatError("block table or payload offset is out of bounds")

    header_copy = bytearray(payload[:header_bytes])
    struct.pack_into("<I", header_copy, 64, 0)

    if zlib.crc32(header_copy) & 0xFFFFFFFF != header_crc:
        raise DatasetFormatError("header checksum mismatch")

    if zlib.crc32(payload[block_table_offset:]) & 0xFFFFFFFF != content_crc:
        raise DatasetFormatError("content checksum mismatch")

    blocks: dict[int, Block] = {}
    ranges: list[tuple[int, int]] = []
    for index in range(block_count):
        offset = block_table_offset + index * block_record_bytes
        kind, block_version, start, size, count, checksum, block_flags = _BLOCK.unpack_from(payload, offset)
        end = start + size

        if not kind or not block_version or block_flags or start < payload_offset or end > len(payload):
            raise DatasetFormatError("sample block is invalid or out of bounds")

        if kind in blocks or any(start < other_end and other_start < end for other_start, other_end in ranges):
            raise DatasetFormatError("sample blocks overlap or repeat a kind")

        data = payload[start:end]
        if zlib.crc32(data) & 0xFFFFFFFF != checksum:
            raise DatasetFormatError("sample block checksum mismatch")

        blocks[kind] = Block(kind, block_version, count, data)
        ranges.append((start, end))

    return Sample(sample_id, generation_id, blocks)
