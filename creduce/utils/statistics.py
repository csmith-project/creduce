class PassStatistic:
    def __init__(self):
        self.stats = {}

    @staticmethod
    def _generate_unique_pass_key(pass_, arg):
        return str(pass_) + str(arg)

    def update(self, pass_, arg, success):
        key = self._generate_unique_pass_key(pass_, arg)

        if key not in self.stats:
            self.stats[key] = {"pass" : pass_,
                               "arg" : arg,
                               "worked" : 0,
                               "failed" : 0}

        if success:
            self.stats[key]["worked"] += 1
        else:
            self.stats[key]["failed"] += 1

    @property
    def sorted_results(self):
        def sort_statistics(item):
            return (-item["worked"], str(item["pass"]), item["arg"])

        return sorted(self.stats.values(), key=sort_statistics)
