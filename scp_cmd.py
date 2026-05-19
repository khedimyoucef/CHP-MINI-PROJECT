import pty
import os
import sys

command = ["scp", "-r", "-o", "StrictHostKeyChecking=no", "-P", "2222", "/mnt/C/Users/Administrator/Desktop/CHP/MINIPROJECT", "slave@127.0.0.1:~/MINIPROJECT"]
password = "youcef16032005\n"

pid, fd = pty.fork()

if pid == 0:
    os.execvp("scp", command)
else:
    try:
        while True:
            data = os.read(fd, 1024)
            if not data:
                break
            sys.stdout.buffer.write(data)
            sys.stdout.buffer.flush()
            if b"password" in data.lower():
                os.write(fd, password.encode())
    except OSError:
        pass
    _, status = os.waitpid(pid, 0)
    sys.exit(os.waitstatus_to_exitcode(status))
