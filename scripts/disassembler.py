#!/usr/bin/env python3
import struct
import json
from pathlib import Path
import argparse

OPMAP_PATH = Path("scripts/omap.json")  # your opcode map

def read_u8(data, offset):
    return data[offset], offset+1

def read_u32(data, offset):
    (v,) = struct.unpack_from("<I", data, offset)
    return v, offset+4

def parse_constants(data, offset, count):
    constants = []
    cur = offset
    for _ in range(count):
        t = data[cur]; cur += 1
        if t == 1:  # int
            v = struct.unpack_from("<i", data, cur)[0]
            cur += 4
            constants.append(("INT", v))
        elif t == 2:  # double
            v = struct.unpack_from("<d", data, cur)[0]
            cur += 8
            constants.append(("DOUBLE", v))
        elif t == 3:  # string
            length, cur = read_u32(data, cur)
            s = data[cur:cur+length].decode('utf-8', errors='replace')
            cur += length
            constants.append(("STRING", s))
        elif t == 4:
            constants.append(("NIL", None))
        elif t == 5:
            b = data[cur]; cur += 1
            constants.append(("BOOL", bool(b)))
    return constants, cur

def decode_operands(code, offset, operands, constants):
    decoded = []
    cur = offset
    for op in operands:
        if op == "u8":
            v, cur = read_u8(code, cur)
            decoded.append(v)
        elif op == "u32":
            v, cur = read_u32(code, cur)
            decoded.append(v)
    return decoded, cur

def disassemble(code, opmap, constants, start_addr=0):
    offset = 0
    while offset < len(code):
        op = code[offset]
        offset += 1
        info = opmap.get(str(op), {"name": f"OP_{op:02X}", "operands":[]})
        operands, offset = decode_operands(code, offset, info["operands"], constants)
        ops_str = " ".join(str(o) for o in operands)
        print(f"{start_addr + offset - 1:04X}: {info['name']} {ops_str}")

def main():
    parser = argparse.ArgumentParser(description="Droplet DBC disassembler")
    parser.add_argument("dbc_file", type=str, help="Path to the DBC file")
    args = parser.parse_args()

    dbc_path = Path(args.dbc_file)
    if not dbc_path.exists():
        print(f"DBC file not found: {dbc_path}")
        return

    data = dbc_path.read_bytes()
    ptr = 0

    if data[0:4] != b"DLBC":
        print("Bad DBC file")
        return
    ptr += 4
    version = data[ptr]; ptr += 1

    const_count, ptr = read_u32(data, ptr)
    constants, ptr = parse_constants(data, ptr, const_count)

    fn_count, ptr = read_u32(data, ptr)
    function_headers = []

    for _ in range(fn_count):
        nameIdx, ptr = read_u32(data, ptr)
        start, ptr = read_u32(data, ptr)
        size, ptr = read_u32(data, ptr)
        argCount = data[ptr]; ptr += 1
        localCount = data[ptr]; ptr += 1
        function_headers.append((nameIdx, start, size, argCount, localCount))

    code_size, ptr = read_u32(data, ptr)
    code = data[ptr:ptr+code_size]

    opmap = json.loads(OPMAP_PATH.read_text())

    # Disassemble each function separately
    for i, (nameIdx, start, size, argCount, localCount) in enumerate(function_headers):
        name = constants[nameIdx][1] if 0 <= nameIdx < len(constants) and constants[nameIdx][0] == "STRING" else f"<fn_{i}>"
        print(f"\n--- Function [{i}] {name} (args={argCount}, locals={localCount}) ---")
        fn_code = code[start:start+size]
        disassemble(fn_code, opmap, constants, start_addr=start)

if __name__ == "__main__":
    main()
