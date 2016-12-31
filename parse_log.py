#!/usr/bin/python

import sys
import os
from re import search
from subprocess import call


RES_LOG = "hash_cmd_res.log"
CMD_LOG = "hash_cmd.log"
INSERT = "INSERT"
REMOVE = "REMOVE"
COMPUTE = "COMPUTE"
UPDATE = "UPDATE"
CONTAINS = "CONTAINS"


class Record:
	def __init__(self, key, cmd):
		self.key = key
		self.cmd = cmd

	def __str__(self):
		print "Key :%d. CMD: %s" %(self.key, self.cmd)
	
	def __repr__(self):
		return "Key :%d. CMD: %s" %(self.key, self.cmd)


def check(rerun):
	passed = 1
	table = {}
	results = {}
	records = {}
	with open(os.path.join(os.getcwd(), RES_LOG)) as res:
		for i,line in enumerate(res):
			if line.rstrip() == '' or line.startswith('#') or len(line) < 10:
				continue
			try:
				thread = int(search(r'(Thread: )(\d+)', line).group(2))
				result = int(search(r'(result: )(\d)', line).group(2))
				# No fucking idea why once in a while a threads index is off by one
				if thread != i or thread in results.keys():
					print "Warning: Thread reported wrong index in result log. line: %d. Try rerunning test to be sure" %(i)
					thread = i
				results[thread] = result
			
			except AttributeError:
				if (rerun):
					print "Rerun results invalid. Try manual rerun"
					sys.exit(1)
				print "Invalid result log file, line: %d. Rerunning test..." %(i)
				reset()

	with open(os.path.join(os.getcwd(), CMD_LOG)) as cmds:
		for line in cmds:
			if line.rstrip() == '' or line.startswith('#'):
				continue
			thread = search(r'(Thread: )(\d+)', line).group(2)
			cmd = search(r'(executing: )(\w+)', line).group(2)
			key = search(r'(Key: )(\d+)', line).group(2)
			records[int(thread)] = Record(int(key), cmd)
	for thread in records.keys():

		if records[thread].cmd == INSERT:
			if records[thread].key in table.keys():
				expected = 0
			else:
				expected = 1
				table[records[thread].key] = 1

		if records[thread].cmd == REMOVE:
			if records[thread].key in table.keys():
				expected = 1
				del table[records[thread].key]
			else:
				expected = 0

		if records[thread].cmd == COMPUTE or records[thread].cmd == UPDATE or records[thread].cmd == CONTAINS:
			if records[thread].key in table.keys():
				expected = 1
			else:
				expected = 0

		if results[thread] != expected:
			print "Failure! Thread: %d result differs. Expected: %d. Result: %d" %(thread, expected, results[thread])
			passed = 0
	
	if passed:
		print "TestHashSync passed with success!"


def reset():
	call(os.path.join(os.getcwd(), "test"), shell=True)
	check(True)
	sys.exit(0)		


def main():
	check(False)
	sys.exit(0)

if __name__ == '__main__':
	main()
