import json
import re
import subprocess


EXPECTED_OUTPUT_CTRL = (
    r"^.*\[info\] karabo\.core\.Device : 'Gotthard2Control' \(version '.*'\)"
    r" with deviceId: '.*' got started on server.*$"
)


EXPECTED_OUTPUT_RECV = (
    r"^.*\[info\] karabo\.core\.Device : 'Gotthard2Receiver' \(version '.*'\)"
    r" with deviceId: '.*' got started on server.*$"
)


def _run_cmd(cmd: str) -> str:
    """
    Runs cmd in the shell, returns its combined stdout/stderr.
    If it times out, kills the server and still returns whatever was in stdout.
    """
    try:
        proc = subprocess.run(
            cmd,
            shell=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            timeout=10,
            check=False,
        )
        # strip a trailing newline
        return proc.stdout.decode().rstrip("\n")
    except subprocess.TimeoutExpired as err:
        # cleanup
        subprocess.run("killall -9 karabo-sls-detector-server", shell=True)
        return err.stdout.decode().rstrip("\n")


def test_gh2_control():
    config = {
        "MyTestGh2Ctrl": {
            "classId": "Gotthard2Control",
            "detectorHostName": ["scs-xox-gh22-det-gotthard2-control2"],
            "udpSrcIp": ["10.253.15.102"],
            "rxHostname": ["scs-rr-sys-con-jungf"],
            "rxTcpPort": [3954],
            "udpDstIp": ["10.253.15.101"],
            "udpDstPort": [50001]}}

    output = _run_cmd(
        f"karabo-sls-detector-server init='{json.dumps(config)}'")
    assert re.findall(EXPECTED_OUTPUT_CTRL, output, re.MULTILINE)


def test_gh2_receiver():
    config = {
        "MyTestGh2Recv": {"classId": "Gotthard2Receiver"}}

    output = _run_cmd(
        f"karabo-sls-detector-server init='{json.dumps(config)}'")
    assert re.findall(EXPECTED_OUTPUT_RECV, output, re.MULTILINE)
