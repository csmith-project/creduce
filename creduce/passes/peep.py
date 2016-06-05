#!/usr/bin/env python3

from .delta import DeltaPass

class PeepDeltaPass(DeltaPass):
    @classmethod
    def check_prerequisites(cls):
        return True

    pass

if __name__ == "__main__":
    #TODO: Add testing functionality!
    print("Run peep pass!")
