#!/usr/bin/env python3

import re

class NestedMatcher:
    @staticmethod
    def find(string, search, start=0, prefix=""):
        if start >= len(string):
            return None

        substring = string[start:]
        m = re.search(prefix + re.escape(search[0]), substring, re.DOTALL)

        if m is None:
            return None

        depth = 1
        pos = m.end() + 1

        while pos < len(substring) and depth > 0:
            if substring[pos] == search[0]:
                depth += 1
            elif substring[pos] == search[1]:
                depth -= 1

            pos += 1

        if pos == len(substring) and depth != 0:
            return None

        return (start + m.start(), start + pos - 1)

if __name__ == "__main__":
    #TODO: Add testing functionality!
    print("Run nested matcher!")
