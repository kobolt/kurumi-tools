#!/usr/bin/python

import serial

class Kurumi(object):
	def __init__(self, port):
		self.s = serial.Serial()
		self.s.port = port
		self.s.baudrate = 9600
		self.s.bytesize = serial.EIGHTBITS
		self.s.parity = serial.PARITY_NONE
		self.s.stopbits = serial.STOPBITS_ONE
		self.s.xonxoff = False
		self.s.rtscts = False
		self.s.dsrdtr = False

		self.s.setDTR(False)
		self.s.open()
		self.s.flushInput()
		self.s.flushOutput()
	
	def __del__(self):
		self.s.close()

	def _command(self, cmd):
		print ">", cmd
		for char in cmd:
			self.s.write(char) # Write one character at a time...
			self.s.read(1)     # ...and read the echo.
		self.s.write("\r") # Finish command...
		self.s.read(10)    # ...and read the prompt.

	def script(self, filename):
		self._command("stop")
		self._command("red off")
		self._command("green off")
		self._command("blue off")
		self._command("clear")
		with open(filename) as fh:
			for line_no, line in enumerate(fh):
				self._command("%s %s" % (line_no, line.strip()))
		self._command("run")

	def blink(self, times):
		self._command("stop")
		self._command("red off")
		self._command("green off")
		self._command("blue off")
		self._command("clear")
		line_no = 0

		if times >= 25:
			for i in range(0, times / 25):
				self._command("%s red on"    % (line_no))
				self._command("%s sleep 100" % (line_no + 1))
				self._command("%s red off"   % (line_no + 2))
				self._command("%s sleep 100" % (line_no + 3))
				line_no += 4
			times = times % 25

		if times >= 5:
			for i in range(0, times / 5):
				self._command("%s green on"  % (line_no))
				self._command("%s sleep 100" % (line_no + 1))
				self._command("%s green off" % (line_no + 2))
				self._command("%s sleep 100" % (line_no + 3))
				line_no += 4
			times = times % 5

		for i in range(0, times):
			self._command("%s blue on"   % (line_no))
			self._command("%s sleep 100" % (line_no + 1))
			self._command("%s blue off"  % (line_no + 2))
			self._command("%s sleep 100" % (line_no + 3))
			line_no += 4

		self._command("%s sleep 1000" % (line_no))
		self._command("run")

if __name__ == "__main__":
	import sys
	k = Kurumi("/dev/ttyUSB0")

	if len(sys.argv) > 1:
		try:
			times = int(sys.argv[1])
		except ValueError:
			times = None

		if times == None:
			k.script(sys.argv[1])
		else:
			k.blink(times)

	else:
		print "Usage: %s <script file | times to blink>" % (sys.argv[0])

