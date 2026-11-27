import io
import json
import unittest
from contextlib import ExitStack, redirect_stdout
from types import SimpleNamespace
from unittest.mock import Mock, call, patch

import serial

from pc import monitor


class Connection:
    def __init__(self, write_error=None):
        self.write = Mock(side_effect=write_error or len)
        self.close = Mock()

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()


class MonitorTests(unittest.TestCase):
    def setUp(self):
        stack = self.enterContext(ExitStack())
        self.output = stack.enter_context(redirect_stdout(io.StringIO()))
        self.open_port = stack.enter_context(patch.object(monitor.serial, "Serial"))
        self.sleep = stack.enter_context(patch.object(monitor.time, "sleep"))
        self.cpu = stack.enter_context(patch.object(monitor.psutil, "cpu_percent"))
        self.ram = stack.enter_context(patch.object(monitor.psutil, "virtual_memory"))
        self.ram.return_value = SimpleNamespace(percent=42.5)

    def assert_stopped(self):
        self.assertIn("モニタリングを終了しました", self.output.getvalue())

    def test_successful_monitoring_keeps_wire_format_and_closes_on_ctrl_c(self):
        connection = Connection()
        self.open_port.return_value = connection
        self.cpu.side_effect = [12.5, 25.0, KeyboardInterrupt]

        monitor.main()

        self.open_port.assert_called_once_with("COM3", 115200, write_timeout=1)
        self.assertEqual(self.cpu.call_args_list, [call(interval=1)] * 3)
        self.assertEqual(self.ram.call_count, 2)
        self.assertEqual(connection.write.call_count, 2)
        for sent, cpu in zip(connection.write.call_args_list, [12.5, 25.0]):
            message = sent.args[0]
            self.assertIsInstance(message, bytes)
            self.assertEqual(message.count(b"\n"), 1)
            self.assertTrue(message.endswith(b"\n"))
            self.assertEqual(json.loads(message), {"cpu": cpu, "ram": 42.5})
        self.sleep.assert_called_once_with(2)
        connection.close.assert_called_once()
        self.assert_stopped()

    def test_missing_port_retries_every_two_seconds_then_sends(self):
        connection = Connection()
        now = 0
        attempts = []

        def sleep(seconds):
            nonlocal now
            now += seconds

        def open_port(*args, **kwargs):
            attempts.append(now)
            if len(attempts) < 4:
                raise serial.SerialException("port missing")
            return connection

        self.sleep.side_effect = sleep
        self.open_port.side_effect = open_port
        self.cpu.side_effect = [10, KeyboardInterrupt]

        monitor.main()

        self.assertEqual(attempts, [0, 2, 4, 6])
        self.assertEqual(self.sleep.call_args_list, [call(2)] * 4)
        connection.write.assert_called_once()
        connection.close.assert_called_once()
        self.assert_stopped()

    def test_disconnect_timeout_and_os_error_close_before_retry_and_resume(self):
        for error in (
            serial.SerialException("USB disconnected"),
            serial.SerialTimeoutException("write timed out"),
            OSError("device removed"),
        ):
            with self.subTest(error=type(error).__name__):
                failed = Connection(write_error=error)
                restored = Connection()
                self.open_port.reset_mock()
                self.sleep.reset_mock()
                self.open_port.side_effect = [failed, restored]
                self.cpu.side_effect = [10, 30, KeyboardInterrupt]

                def sleep(seconds):
                    if self.open_port.call_count == 1 and failed.write.called:
                        failed.close.assert_called_once()

                self.sleep.side_effect = sleep
                monitor.main()

                self.assertEqual(self.open_port.call_count, 2)
                self.assertEqual(self.sleep.call_args_list, [call(2)] * 3)
                restored.write.assert_called_once_with(b'{"cpu": 30, "ram": 42.5}\n')
                failed.close.assert_called_once()
                restored.close.assert_called_once()
                self.assert_stopped()

    def test_repeated_disconnects_recover_each_time(self):
        connections = [Connection(), Connection(), Connection()]
        for connection in connections[:2]:
            outcomes = iter([None, serial.SerialException("unplugged")])

            def write(message, outcomes=outcomes):
                outcome = next(outcomes)
                if isinstance(outcome, Exception):
                    raise outcome
                return len(message)

            connection.write.side_effect = write
        self.open_port.side_effect = connections
        self.cpu.side_effect = [1, 2, 3, 4, 5, KeyboardInterrupt]

        monitor.main()

        self.assertEqual(self.open_port.call_count, 3)
        self.assertEqual(self.sleep.call_args_list, [call(2)] * 5)
        for connection in connections:
            connection.close.assert_called_once()
        self.assertEqual(json.loads(connections[-1].write.call_args.args[0])["cpu"], 5)

    def test_short_write_is_retried(self):
        failed = Connection()
        failed.write.side_effect = lambda message: len(message) - 1
        restored = Connection()
        self.open_port.side_effect = [failed, restored]
        self.cpu.side_effect = [10, 20, KeyboardInterrupt]

        monitor.main()

        self.assertEqual(self.open_port.call_count, 2)
        failed.close.assert_called_once()
        restored.write.assert_called_once_with(b'{"cpu": 20, "ram": 42.5}\n')

    def test_ctrl_c_during_open_exits_without_retry(self):
        self.open_port.side_effect = KeyboardInterrupt

        monitor.main()

        self.open_port.assert_called_once()
        self.sleep.assert_not_called()
        self.assert_stopped()

    def test_ctrl_c_during_retry_wait_exits(self):
        self.open_port.side_effect = serial.SerialException("missing")
        self.sleep.side_effect = KeyboardInterrupt

        monitor.main()

        self.open_port.assert_called_once()
        self.sleep.assert_called_once_with(2)
        self.assert_stopped()

    def test_ctrl_c_during_startup_wait_closes_port(self):
        connection = Connection()
        self.open_port.return_value = connection
        self.sleep.side_effect = KeyboardInterrupt

        monitor.main()

        connection.close.assert_called_once()
        connection.write.assert_not_called()
        self.assert_stopped()

    def test_ctrl_c_during_write_closes_port(self):
        connection = Connection(write_error=KeyboardInterrupt)
        self.open_port.return_value = connection
        self.cpu.return_value = 10

        monitor.main()

        connection.close.assert_called_once()
        self.sleep.assert_called_once_with(2)
        self.assert_stopped()

    def test_os_error_on_open_retries(self):
        self.open_port.side_effect = [OSError("unavailable"), KeyboardInterrupt]

        monitor.main()

        self.assertEqual(self.open_port.call_count, 2)
        self.sleep.assert_called_once_with(2)
        self.assert_stopped()


if __name__ == "__main__":
    unittest.main()
