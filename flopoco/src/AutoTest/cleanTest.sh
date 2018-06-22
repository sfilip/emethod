rm TestResults/tmp/*
nbTests=$(grep -c flopoco TestResults/$1/report)
nbErrors=$(grep -c ERROR TestResults/$1/report)
nbVHDL=$(grep -c "VHDL generated" TestResults/$1/report)
nbSuccess=$((nbVHDL-nbErrors))
rateError=$(((nbErrors*100)/nbTests))
rateSuccess=$(((nbSuccess*100)/nbTests))
rateVHDL=$(((nbVHDL*100)/nbTests))
echo "VHDL : $rateVHDL% generated" >> TestResults/report
echo "Error : $rateError%" >> TestResults/report
echo "Success : $rateSuccess%" >> TestResults/report
echo "See report in TestResults/$1 for details" >> TestResults/report
echo "VHDL : $rateVHDL% generated"
echo "Error : $rateError%"
echo "Success : $rateSuccess%"
echo "See report in TestResults/$1 for details"