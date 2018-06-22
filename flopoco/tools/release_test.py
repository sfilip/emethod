# Usage:  -n option for new release test generation. 
# no parameter continues from where it left-off
# Log file in  the release_test.res file
# Completed files have an '@' on the first line, that's how the script
# knows that they are done. Before starting a fresh release it is
# recommended to flush all the files and recover them from the svn.

# TODO remove commands and replace the remaining uses  with subprocess

import random
import math
import os
import subprocess
import string
import commands
import pipes
import time
import sys
import fileinput



test_cases_per_combination = 1000
useModelSim=False #False # if True, use modelsim; if False, use ghdl
testBench = "TestBench"   #one of TestBench or TestBenchFile
timeUntilDiscardTest = 50

#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------

#prints the operator parameters and their types nicely
def printResults(list, lfile):
	i=0
	print '%93s|%12s|%11s|%4s' % ("TEST", "GENERATE_HDL", "COMPILE HDL", "PASS") 
	lfile.write( '%93s|%12s|%11s|%4s\n' % ("TEST", "GENERATE_HDL", "COMPILE HDL", "PASS")) 
	print "----------------------------------------------------------------------------------------------------"
	lfile.write("----------------------------------------------------------------------------------------------------\n")
	while (i)<len(list):
		print '%93s|%12s|%11s|%4s' % (list[i][0], list[i][1], list[i][2], list[i][3]) 
		lfile.write('%93s|%12s|%11s|%4s' % (list[i][0], list[i][1], list[i][2], list[i][3]))
		lfile.write("\n")
		i=i+1
	return

#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------

def pre_append(line, file_name):
    fobj = fileinput.FileInput(file_name, inplace=1)
    first_line = fobj.readline()
    sys.stdout.write("%s\n%s" % (line, first_line))
    for line in fobj:
        sys.stdout.write("%s" % line)
    fobj.close()
#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------



#/* main */
if __name__ == '__main__':

	if (len(sys.argv)==2):
		option = sys.argv[1] #if -n we do a clean test
	else:
		print "-n option is for starting a new regression test. all results will be cleared\n"
		option = ""
	res = []
	#REMOVE TEMPORARY MODELSIM FILES
	os.system("rm -f wlf*")

	#CREATE A LOG FILE 
	if ( option == "-n" ) :
		logfile = open( "release_test.log","w")
		logfile.close()

	logfile = open( "release_test.log","a")
		
	if ( option == "-n" ): 
		resultfile = open( "release_test.res", "w")
		resultfile.close()
	
	pass_all = True
	#PARSE EXTERNAL TEST FILE
	print("Parsing operators from external list: \n")
	logfile.write("Parsing operators from external list: \n")

	for filename in os.listdir("tests/"):
		if ((filename[len(filename)-1]!='~') and (filename[0]!='.')):
			print "processing " + filename + " ... "
		
			fd = open ("tests/"+filename)
			if fd < 0:
				print "Unable to open file ".filename
				logfile.write("Unable to open file " + filename)

			completedFile = False
			skipfile = False
			for line in fd:
				if ((line[0]=='@') and (option!="-n")):
					completedFile = True
					print "Skipping this file. It was completed before the crash"

				if (line[0]=='!'):
					skipfile = True
					print "Skipping the rest of the file "+filename+"; ! encountered"
			
				if ((line[0]!='#') and (len(line)>1) and (skipfile == False) and (completedFile == False)):
					resultfile = open( "release_test.res", "a")

					run_cmd = line[:len(line)-1] + " " + testBench + " n=" + `test_cases_per_combination`
					print run_cmd
					logfile.write(run_cmd+"\n")
					modelsim_food = commands.getoutput(run_cmd)
					did_generate_vhdl = True
					status = string.find(modelsim_food, "Output file: flopoco.vhdl")

					if status < 0:
						did_generate_vhdl = False
						print("Did not generate VHDL");

					if useModelSim:
						commands.getoutput("rm -f vsim*")
						commands.getoutput("killall -9 vsimk")
						modelsim_food = modelsim_food[string.find(modelsim_food, "vdel") : string.find(modelsim_food, "To run the simulation using gHDL,")-1 ]
					else:
						ghdl_food = modelsim_food[string.find(modelsim_food, "   ghdl") : string.find(modelsim_food, "Final")-1 ]
	
					finished = False
					pass_test = True
					did_compile = True

					if did_generate_vhdl:
						print("It did generate VHDL");

						if useModelSim:
						#start modelsim
							p = subprocess.Popen("vsim -c", shell=True, bufsize=1, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, close_fds=True)
							(child_stdin, child_stdout, child_stderr) = (p.stdin, p.stdout, p.stderr)
							child_stdin.write("vdel -all -lib work\n")
							child_stdin.write('vlib work\n')
							child_stdin.write('vcom flopoco.vhdl \n')
							child_stdin.write('vcom flopoco.vhdl \n')
							child_stdin.write(modelsim_food[string.find(modelsim_food,"vsim"):string.find(modelsim_food,"add")-1]+"\n" )
							child_stdin.write('add wave -r * \n')
							child_stdin.write(modelsim_food[string.find(modelsim_food,"run"):]+"\n")
							child_stdin.write('exit \n')
#							timer set-up
							startTime = time.clock()
							timeUp = False						
														
							while ((not finished) and (did_compile) and (not timeUp)):
								currentTime = time.clock()	
								#print "the start time was:" + str(startTime)
								#print "the currentTime time was" + str(currentTime)
								if (currentTime - startTime > float(timeUntilDiscardTest)):
									timeUp = True
									
								st = child_stdout.readline()
								#print st[0:len(st)-2]
								logfile.write(st[0:len(st)-2]+"\n")
								status = string.find(st, "Error:")
								if status > 0:
									pass_test = False
									#did_compile = False
						
									status = string.find(st, "Incorrect")
									if status > 0:
										pass_test = False
		
								status = string.find(st, "Stopped")
								if status > 0:
									finished = True
									#did_compile = False
		
									status = string.find(st, "Error loading design")
									if status > 0:
										did_compile = False
										finished = True
								
	

							if timeUp:
								print "some bad thing happended!!!"

							if did_compile:
								child_stdin.write('exit \n')
								child_stdout.close()
								child_stdin.close()
								child_stderr.close()

						else: # use ghdl
							time.sleep(1)
							cmd=ghdl_food[string.find(ghdl_food,"ghdl -a"):string.find(ghdl_food,".vhdl")+5]
							logfile.write(cmd+"\n")
							print(cmd)							
							try:
								status=subprocess.check_output(cmd,shell=True)
							except subprocess.CalledProcessError, e:
								errorstring=e.output
								print "ghdl -a error:"
								print errorstring
								logfile.write(errorstring+"\n")
								did_compile=False
								pass_test=False

							cmd=ghdl_food[string.find(ghdl_food,"ghdl -e"):string.find(ghdl_food,"   ghdl -r")-1]
							logfile.write(cmd+"\n")
							print(cmd)
							try:
								status=subprocess.check_output(cmd,shell=True)
							except subprocess.CalledProcessError, e:
								errorstring=e.output
								print "ghdl -e error:"
								print errorstring
								logfile.write(errorstring+"\n")
								did_compile=False
								pass_test=False
									
							cmd=ghdl_food[string.find(ghdl_food,"ghdl -r"):string.find(ghdl_food,".vcd")+4]
							logfile.write(cmd+"\n")
							print(cmd)
							try:
								status=subprocess.check_output(cmd,stderr=subprocess.STDOUT,shell=True)
							except subprocess.CalledProcessError, e:
								errorstring=e.output
								if string.find(errorstring, "Incorrect") !=-1:
									print "ghdl -r error:"
									print errorstring
									logfile.write(errorstring+"\n")
									pass_test=False

							subprocess.call("rm *.vcd e~testbench* testbench* *.svg", shell=True)

					pass_test = pass_test and did_generate_vhdl
					res.append([run_cmd, `did_generate_vhdl`, `did_compile`, `pass_test`])
					
					res_line = run_cmd + ", " + str(did_generate_vhdl) + ", " + str(did_compile) + ", " + str(pass_test) + "\n"
					resultfile.write( res_line )
					resultfile.close()
					pass_all = pass_all and pass_test
		
					if pass_test:
						print "Test: ", run_cmd , " has SUCCEDED "
					else:
						print "Test: ", run_cmd , " has FAILED "

			fd.close()
			
			if (completedFile == False):
				pre_append( "@", "tests/"+filename); 

	printResults( res , logfile)

	print "FINAL PASS STATUS: ", `pass_all`
	logfile.write("FINAL PASS STATUS:" +  `pass_all`)

	logfile.close()
	print "Logfile was written to: release_test.log"

