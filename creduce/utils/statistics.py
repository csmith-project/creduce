class PassStatistic:
    def __init__(self):
        self.stats = {}

    def update(self, pass_, success):
        key = repr(pass_)

        if key not in self.stats:
            self.stats[key] = {"pass" : pass_,
                               "worked" : 0,
                               "failed" : 0}

        if success:
            self.stats[key]["worked"] += 1
        else:
            self.stats[key]["failed"] += 1

    @property
    def sorted_results(self):
        def sort_statistics(item):
            return (-item["worked"], str(item["pass"]))

        return sorted(self.stats.values(), key=sort_statistics)
