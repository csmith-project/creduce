import sys

if sys.platform == "win32":
    import msvcrt
else:
    import termios
    import select
    import weakref

class KeyLogger:
    def __init__(self):
        if sys.platform != "win32":
            fd = sys.stdin.fileno()
            new_term = termios.tcgetattr(fd)
            old_term = termios.tcgetattr(fd)

            self._finalizer = weakref.finalize(self, termios.tcsetattr, fd, termios.TCSAFLUSH, old_term)

            # New terminal setting unbuffered
            new_term[3] = new_term[3] & ~termios.ICANON & ~termios.ECHO
            termios.tcsetattr(fd, termios.TCSAFLUSH, new_term)

    def _getch(self):
        if sys.platform == "win32":
            return msvcrt.getch().decode("utf-8")
        else:
            return sys.stdin.read(1)

    def _kbhit(self):
        if sys.platform == "win32":
            return msvcrt.kbhit()
        else:
            (dr, dw, de) = select.select([sys.stdin], [], [], 0)
            return (dr != [])

    def pressed_key(self):
        if self._kbhit():
            return self._getch()
        else:
            return None
