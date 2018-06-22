##
################################################################################
##             Compilation and Result Generation script for Vivado
## This tool is part of  FloPoCo
## Author:  Florent de Dinechin, 2016
## All rights reserved
################################################################################

import os
import sys
import re
import string
import argparse

def report(text):
    print "vivado_runsyn: " + text

    
def get_compile_info(filename):
    vhdl=open(filename).read()

    # last entity
    endss = [match.end() for match in re.finditer("entity", vhdl)] # list of endpoints of match of "entity"
    last_entity_name_start = endss[-2] +1 # skip the space
    i = last_entity_name_start
    while(vhdl[i]!=" "):
        i=i+1
    last_entity_name_end = i
    entityname=vhdl[last_entity_name_start:last_entity_name_end]

    # target 
    endss = [match.end() for match in re.finditer("-- VHDL generated for", vhdl)] # list of endpoints of match of "entity"
    target_name_start = endss[-1] +1
    i = target_name_start
    while(vhdl[i]!=" "):
        i=i+1
    target_name_end = i
    targetname=vhdl[target_name_start:target_name_end]
    
    # the frequency follows but we don't need to read it so far
    frequency_start=target_name_end+3 #  skip " @ "
    i = frequency_start
    while(vhdl[i]!=" " and vhdl[i]!="M"): # 400MHz or 400 MHz
        i=i+1
    frequency_end = i
    frequency = vhdl[frequency_start:frequency_end]

    return (entityname, targetname, frequency)



#/* main */
if __name__ == '__main__':


    parser = argparse.ArgumentParser(description='This is an helper script for FloPoCo that launches Xilinx Vivado and extracts resource consumption and critical path information')
    parser.add_argument('-i', '--implement', action='store_true', help='Go all the way to implementation (default stops after synthesis)')
    parser.add_argument('-v', '--vhdl', help='VHDL file name (default flopoco.vhdl)')
    parser.add_argument('-e', '--entity', help='Entity name (default is last entity of the VHDL file)')
    parser.add_argument('-t', '--target', help='Target name (default is read from the VHDL file)')
    parser.add_argument('-f', '--frequency', help='Objective frequency (default is read from the VHDL file)')

    options=parser.parse_args()


    if (options.implement==True):
        synthesis_only=False
    else:
        synthesis_only=True

    if (options.vhdl==None):
        filename = "flopoco.vhdl"
    else:
        filename = options.vhdl

    # Read from the vhdl file the entity name and target hardware and freqyency
    report("Reading file " + filename)
    (entity_in_file, target_in_file, frequency_in_file) =  get_compile_info(filename)

    if (options.entity==None):
        entity = entity_in_file
    else:
        entity = options.entity

    if (options.target==None):
        target = target_in_file
    else:
        target = options.target

    if (options.frequency==None):
        frequency = frequency_in_file
    else:
        frequency = options.frequency


    report("   entity:     " +  entity)
    report("   target:     " +  target)
    report("   frequency:  " +  frequency + " MHz")

    if target.lower()=="kintex7":
        part="xc7k70tfbv484-3"
    elif target.lower()=="zynq7000":
        part="xc7z020clg484-1"
    elif target.lower()=="virtex6" or target.lower()=="virtex5":
        print("**************** Warning ************************************") 
        print("Pre-series7 are unsupported by Vivado: using Kintex7 instead") 
        print("*************************************************************") 
        part="xc7k70tfbv484-3"
    else:
        raise BaseException("Target " + target + " not supported")
        
    workdir="/tmp/vivado_runsyn_"+entity+"_"+target+"_"+frequency
    os.system("rm -R "+workdir)
    os.mkdir(workdir)
    os.system("cp flopoco.vhdl "+workdir)
    os.chdir(workdir)
    

    # Create the clock.xdc file. For this we use the previous frequency,
    # but since I am too lazy to parse the VHDL to find inouts and output names, I copy the set_input_delay etc from the FloPoCo generated file
    # This file was created by FloPoCo to be used by the vivado_runsyn utility. Sorry to clutter your tmp.
    xdc_file_name = workdir+"/clock.xdc" 
    report("writing "+xdc_file_name)
    tmp_clock_file = open("/tmp/clock.xdc","r") # this is the one created by FloPoCo
    clock_file = open(xdc_file_name,"w")
    period = 1000.0/float(frequency) # the frequency is in MHz and the period in ns
    clock_file.write("create_clock -name clk -period " + str(period) + " \n")
    tmp_clock_file.readline() # skip the two first lines
    tmp_clock_file.readline()
    for line in tmp_clock_file:
        clock_file.write(line)
    clock_file.close()
    tmp_clock_file.close()

    project_name = "test_" + entity
    tcl_script_name = os.path.join(workdir, project_name+".tcl")
    filename_abs = os.path.abspath(filename)

    # First futile attempt to get a timing report

    report("writing " +  tcl_script_name)
    tclscriptfile = open( tcl_script_name,"w")
    tclscriptfile.write("# Synthesis of " + entity + "\n")
    tclscriptfile.write("# Generated by FloPoCo's vivado-runsyn.py utility\n")
    tclscriptfile.write("create_project " + project_name + " -part " + part + "\n")
#    tclscriptfile.write("set_property board_part em.avnet.com:zed:part0:1.3 [current_project]\n")
    tclscriptfile.write("add_files -norecurse " + filename_abs + "\n")
    tclscriptfile.write("read_xdc " + xdc_file_name + "\n")
    tclscriptfile.write("update_compile_order -fileset sources_1\n")
    tclscriptfile.write("update_compile_order -fileset sim_1\n")

    # Reporting files
    utilization_report_file = workdir + "/"  + entity + "_utilization_"
    if synthesis_only:
        utilization_report_file +="synth.rpt"
    else:
        utilization_report_file +="placed.rpt"

    if synthesis_only:
        result_name = "synth_1"
        # The following command is great because it remove the OBUFs but where to get the area?
        tclscriptfile.write("synth_design  -mode out_of_context \n")
    else:
        result_name = "impl_1"
        tclscriptfile.write("launch_runs " + result_name + "\n")
        tclscriptfile.write("wait_on_run " + result_name + "\n")
        tclscriptfile.write("open_run " + result_name + " -name " + result_name + "\n")
    tclscriptfile.write("report_utilization  -file "+  utilization_report_file +"\n")


        
#    tclscriptfile.write("report_timing_summary -delay_type max -report_unconstrained -check_timing_verbose -max_paths 10 -input_pins -file " + os.path.join(workdir, project_name+"_timing_report.txt") + " \n")

    # Timing report
    timing_report_file = workdir + "/" + project_name  + entity + "_timing_"
    if synthesis_only:
        timing_report_file +="synth.rpt"
    else:
        timing_report_file +="placed.rpt"

    tclscriptfile.write("report_timing -file " + timing_report_file + " \n")
        
    tclscriptfile.close()

    vivado_command = ("vivado -mode batch -source " + tcl_script_name)
    report("Running Vivado: " + vivado_command)
    os.system(vivado_command)


    report("cat " + utilization_report_file)
    os.system("cat " + utilization_report_file)
    report("cat " + timing_report_file)
    os.system("cat " + timing_report_file)

    
    
    
#    p = subprocess.Popen(vivado_command, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, close_fds=True)
