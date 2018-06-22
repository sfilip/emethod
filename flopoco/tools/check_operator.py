import string
import subprocess
import sys
import time

test_cases_per_combination = 1000
testbench = "TestBenchFile"
final_test = True
path = "./flopoco"

def checkOp ( cmd_operator ):

        error_msg = "~~~~~ " + cmd_operator + " ~~~~~" + "\n"
        #initialize all at true
	pass_test = True
	#Create a logfile to report errors only
	errorlogfile = open ( "soaktest.errors.log", "a" )
	#Create a logfile to report all
	logfile = open ( "soaktest.full.log", "a" )
        # progress report on std_out

	#launch the flopoco + TestBench command
	cmd = path + " " + cmd_operator + " " + testbench + " " + `test_cases_per_combination`
        print " > " + cmd
        logfile.write (cmd + "\n" )
        errorlogfile.write (cmd + "\n" )

	blah =  subprocess.check_output(cmd, shell=True, stderr = subprocess.STDOUT)
        logfile.write(blah + "\n" )

	status = string.find ( blah, "Output file: flopoco.vhdl" )
	if status < 0 :
		did_generate_vhdl = False
                pass_test = False
                print "VHDL did not generate"
                errorlogfile.write (cmd + "\n\n" )
                errorlogfile.write(blah + "\n" )
        else :
                # VHDL generated, now pass it through ghdl
                did_generate_vhdl = True


                ghdl_food = blah [ string.find ( blah, "ghdl" ) : string.find ( blah, "Final" ) - 1 ]

                # wait one second, otherwise ghdl will crash. Don't know why
                subprocess.check_output("sleep 1", shell=True, stderr = subprocess.STDOUT)




		#now we can test the three ghdl command
		cmd = ghdl_food [ string.find ( ghdl_food, "ghdl -a" ) : string.find ( ghdl_food, "ghdl -e" ) -1 ]
                print " > " + cmd
                logfile.write (cmd + "\n" )
                        
		try:
                        did_compile = True
			blah = subprocess.check_output ( cmd, shell = True, stderr = subprocess.STDOUT )
                        logfile.write(blah + "\n" )
		except subprocess.CalledProcessError as e:
                        blah=e.output
                        logfile.write(blah + "\n" )
                        errorlogfile.write ("ERROR in " + cmd + "\n" )
                        print "ERROR in ghdl -a"
			did_compile = False
			pass_test = False
		




		if did_compile :
                        # Now ghdl -e
			cmd = ghdl_food [ string.find ( ghdl_food, "ghdl -e" ) : string.find ( ghdl_food, "ghdl -r" ) - 1 ]
                        print " > " + cmd
                        logfile.write (cmd + "\n" )
			try:
				blah = subprocess.check_output ( cmd, stderr = subprocess.STDOUT, shell = True )
                                logfile.write(blah + "\n" )
			except subprocess.CalledProcessError as e:
                                blah=e.output
                                logfile.write(blah + "\n" )
                                errorlogfile.write ("ERROR in " + cmd + "\n" )
                                print "ERROR in ghdl -e"
				did_compile = False
				pass_test = False
			
			if did_compile :
				cmd = ghdl_food [ string.find ( ghdl_food, "ghdl -r" ) : string.find ( ghdl_food, "gtkwave" ) - 1]
				print " > " + cmd
                                logfile.write (cmd + "\n" )
                                blah = subprocess.check_output ( cmd, stderr = subprocess.STDOUT, shell = True )
                                logfile.write(blah + "\n" )
                                if string.find ( blah, "Incorrect" ) != -1 :
                                        errorlogfile.write ("ERROR in " + cmd + "\n" )
                                        print "ERROR in ghdl -r"
                                        pass_test = False
	       
	subprocess.call ( "rm -f *.vcd e~testbench* testbench* *.svg", shell = True )

	final_test = did_generate_vhdl and pass_test
	
	#write in the logfile only if error happened
	if not final_test :
		logfile.write ( error_msg + "\n" )

	logfile.close ()
