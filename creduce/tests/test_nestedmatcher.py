import unittest

from ..utils.nestedmatcher import find, search, BalancedExpr, RegExPattern, BalancedPattern, OrPattern

class SimpleParensTest(unittest.TestCase):
    def test_1(self):
        m = find(BalancedExpr.parens, "No match in this string!")
        self.assertIsNone(m)

    def test_2(self):
        m = find(BalancedExpr.parens, "No match in this string!", pos=500)
        self.assertIsNone(m)

    def test_3(self):
        m = find(BalancedExpr.parens, "No match in this string!", pos=500)
        self.assertIsNone(m)

    def test_4(self):
        m = find(BalancedExpr.parens, "No match in this string!", pos=-1)
        self.assertIsNone(m)

    def test_5(self):
        m = find(BalancedExpr.parens, "This string contains a (simple match)!")
        self.assertEqual(m, (23, 37))

    def test_6(self):
        m = find(BalancedExpr.parens, "This string contains a (simple match at the end!)")
        self.assertEqual(m, (23, 49))

    def test_7(self):
        m = find(BalancedExpr.parens, "(This string contains) a simple match at the beginning!")
        self.assertEqual(m, (0, 22))

    def test_8(self):
        m = find(BalancedExpr.parens, "(This string contains) two (simple matches)!")
        self.assertEqual(m, (0, 22))

    def test_9(self):
        m = find(BalancedExpr.parens, "(This (string) contains) two nested matches!")
        self.assertEqual(m, (0, 24))

    def test_10(self):
        m = find(BalancedExpr.parens, "(This (string) contains) two nested matches!", pos=1)
        self.assertEqual(m, (6, 14))

    def test_11(self):
        m = find(BalancedExpr.parens, "(This (string) contains) two nested matches!", prefix="This ")
        self.assertEqual(m, (1, 14))

class ComplexParensTest(unittest.TestCase):
    def test_1(self):
        parts = [RegExPattern(r"This\s"), (BalancedPattern(BalancedExpr.parens), "nested")]
        m = search(parts, "(This (string) contains) two nested matches!")
        self.assertEqual(m, {"all" : (1, 14), "nested" : (6, 14)})

    def test_2(self):
        parts = [RegExPattern(r"This\s"),
                 (BalancedPattern(BalancedExpr.parens), "nested"),
                 RegExPattern(r"[^(]*"),
                 (BalancedPattern(BalancedExpr.parens), "nested2")]
        m = search(parts, "(This (string) contains) two (nested) matches!")
        self.assertEqual(m, {"all" : (1, 37), "nested" : (6, 14), "nested2" : (29, 37)})

    def test_3(self):
        parts = [RegExPattern(r"[Tt]his\s"),
                 (BalancedPattern(BalancedExpr.parens), "nested"),
                 RegExPattern(r"[^(]*"),
                 (BalancedPattern(BalancedExpr.parens), "nested2")]
        m = search(parts, "(This string this (contains)) two (nested) matches!")
        self.assertEqual(m, {"all" : (13, 42), "nested" : (18, 28), "nested2" : (34, 42)})

    def test_4(self):
        parts = [RegExPattern(r"[Tt]his\s"),
                 (BalancedPattern(BalancedExpr.parens), "nested"),
                 RegExPattern(r"[^(]*"), # consumes also "two"
                 (OrPattern(BalancedPattern(BalancedExpr.parens), RegExPattern("two")), "nested2")]
        m = search(parts, "(This string this (contains)) two (nested) matches!")
        self.assertEqual(m, {"all" : (13, 42), "nested" : (18, 28), "nested2" : (34, 42)})

    def test_5(self):
        parts = [RegExPattern(r"[Tt]his\s"),
                 (BalancedPattern(BalancedExpr.parens), "nested"),
                 RegExPattern(r"[^(t]*"),
                 (OrPattern(BalancedPattern(BalancedExpr.parens), RegExPattern("two")), "nested2")]
        m = search(parts, "(This string this (contains)) two (nested) matches!")
        self.assertEqual(m, {"all" : (13, 33), "nested" : (18, 28), "nested2" : (30, 33)})
