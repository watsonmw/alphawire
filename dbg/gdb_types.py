import gdb

class MStrPrinter:
    """Print an MStr."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        m_str = self.val['str']
        size = int(self.val['size'])
        if not m_str:
            return "NULL"
        return m_str.string('utf-8', 'ignore', size)

    def display_hint(self):
        return 'string'

class MStrViewPrinter:
    """Print an MStrView."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        m_str = self.val['str']
        size = int(self.val['size'])
        if not m_str:
            return "NULL"
        return m_str.string('utf-8', 'ignore', size)

    def display_hint(self):
        return 'string'

def build_pretty_printer():
    pp = gdb.printing.RegexpCollectionPrettyPrinter("alphawire")
    pp.add_printer('MStr', '^MStr$', MStrPrinter)
    pp.add_printer('MStrView', '^MStrView$', MStrViewPrinter)
    return pp

def register_printers(obj):
    gdb.printing.register_pretty_printer(obj, build_pretty_printer())

register_printers(gdb.current_objfile())