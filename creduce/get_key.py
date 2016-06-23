import time
import sys
import sys
import tty
import termios
import select

def get_key():

    (ready, _, _) = select.select([fd], [], [], 0)

    if ready:
        key = sys.stdin.read(1)
    else:
        key = None

    return key

if __name__ == "__main__":
    fd = sys.stdin.fileno()
    old_settings = termios.tcgetattr(fd)
    tty.setraw(sys.stdin.fileno())

    i = 0

    while True:
        i+= 1

        while i % 1000 != 0:
            i+=1

        sys.stdout.write("\a")

        key = get_key()

        if key:
            print("Key: " + key)
            break

    termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
