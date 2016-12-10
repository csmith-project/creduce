class ExternalPrograms:
    programs = {
            "topformflat" : "topformflat",
            "clang_delta" : "clang_delta",
            "unifdef" : "unifdef",
            "clang-format" : "clang-format",
            "clex" : "clex"
            }

    def __init__(self):
        self.__programs = ExternalPrograms.programs.copy()

    def __getitem__(self, key):
        return self.__programs[key]

    def __setitem__(self, key, value):
        self.__programs[key] = value

    def __delitem(self, key):
        self.__programs[key] = ExternalPrograms.programs[key]
