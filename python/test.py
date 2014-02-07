import PyPxarCore
from PyPxarCore import Pixel, PixelConfig, PyPxarCore


# collect some basic settings

# Signal delays
sig_delays = {
"clk":2,
"ctr":2,
"sda":17,
"tin":7,
"deser160phase":4}

# Power settings:
power_settings = {
"va":1.9,
"vd":2.6,
"ia":1.190,
"id":1.10}

# Pattern Generator for single ROC operation:
pg_setup = {
0x0800:25,    # PG_RESR
0x0400:101+5, # PG_CAL
0x0200:16,    # PG_TRG
0x0100:0}     # PG_TOK

# Start an API instance from the core pXar library
api = PyPxarCore(usbId="*",logLevel="DEBUGAPI")
print api.getVersion()
if not api.initTestboard(pg_setup = pg_setup, 
                         power_settings = power_settings, 
                         sig_delays = sig_delays):
    print "WARNING: could not init DTB -- possible firmware mismatch."
    print "Please check if a new FW version is available"
    exit

tbmDACs = [{
        "clear":0xF0,       # Init TBM, Reset ROC
        "counters":0x01,    # Disable PKAM Counter
        "mode":0xC0,        # Set Mode = Calibration
        "pkam_set":0x10,    # Set PKAM Counter
        "delays":0x00,      # Set Delays
        "temperature": 0x00 # Turn off Temperature Measurement
        }]

print "Have DAC config for " + str(len(tbmDACs)) + " TBMs:"
for idx, tbmDAC in enumerate(tbmDACs):
    for key in tbmDAC:
        print "  TBM dac: " + str(key) + " = " + str(tbmDAC[key])


dacs = {
    "Vdig":7,
    "Vana":84,
    "Vsf":30,
    "Vcomp":12,
    "VwllPr":60,
    "VwllSh":60,
    "VhldDel":230,
    "Vtrim":29,
    "VthrComp":86,
    "VIBias_Bus":1,
    "Vbias_sf":6,
    "VoffsetOp":40,
    "VOffsetRO":129,
    "VIon":120,
    "Vcomp_ADC":100,
    "VIref_ADC":91,
    "VIbias_roc":150,
    "VIColOr":50,
    "Vcal":220,
    "CalDel":122,
    "CtrlReg":4,
    "WBC":100
    }

pixels = list()
for column in range(0, 51):
    for row in range(0, 79):
        p = PixelConfig(column,row,15)
        #p.enable = True
        pixels.append(p)

print "And we have just initialized " + str(len(pixels)) + " pixel configs to be used for every ROC!"

rocPixels = list()
rocDacs = list()
for rocs in range(0,15):
    rocPixels.append(pixels)
    rocDacs.append(dacs)

api.initDUT("tbm08",tbmDACs,"psi46digv2",rocDacs,rocPixels)

print "pixel 35 is " + str(pixels[35].enable)

api.dut.status()
api.dut.testAllPixels(True)
print " enabled all pixels"
api.getPulseheightVsDAC("vcal", 15, 32)
