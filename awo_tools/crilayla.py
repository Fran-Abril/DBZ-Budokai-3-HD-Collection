#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""crilayla.py - descompresor CRILAYLA (los ficheros de los CPK de CRI: Xenoverse, SDBH...). studies 9b.
Algoritmo publico (LZ que se lee de atras hacia delante; los 0x100 primeros bytes de la salida van sin comprimir
al final del bloque). decompress(bytes) -> bytes; sin la marca CRILAYLA devuelve los datos tal cual."""
import struct


def decompress(data):
    data = bytes(data)
    if data[:8] != b"CRILAYLA":
        return data
    size, hdr_off = struct.unpack_from("<II", data, 8)
    out = bytearray(size + 0x100)
    out[:0x100] = data[hdr_off + 0x10:hdr_off + 0x10 + 0x100]
    pos = len(data) - 0x100 - 1
    end = 0x100 + size - 1
    pool = left = produced = 0
    vle = (2, 3, 5, 8)

    def bits(n):
        nonlocal pos, pool, left
        v = got = 0
        while got < n:
            if left == 0:
                pool, left, pos = data[pos], 8, pos - 1
            t = min(left, n - got)
            v = (v << t) | ((pool >> (left - t)) & ((1 << t) - 1))
            left -= t
            got += t
        return v

    while produced < size:
        if bits(1):
            off = bits(13) + 3
            ln = 3
            for i, b in enumerate(vle):
                lv = bits(b)
                ln += lv
                if lv != (1 << b) - 1:
                    break
            else:
                while True:
                    lv = bits(8)
                    ln += lv
                    if lv != 255:
                        break
            for _ in range(ln):
                out[end - produced] = out[end - produced + off]
                produced += 1
        else:
            out[end - produced] = bits(8)
            produced += 1
    return bytes(out)


if __name__ == "__main__":
    import sys
    sys.stdout.write("uso: from crilayla import decompress\n")
