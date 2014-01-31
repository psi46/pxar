import PyPxarCore
from PyPxarCore import Pixel
p = Pixel()
p.decode(12345)
print p.column
p2 = Pixel()
print p2.column

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
from PyPxarCore import PyPxarCore
api = PyPxarCore()
print api.getVersion()
api.initTestboard(pg_setup = pg_setup, 
                  power_settings = power_settings, 
                  sig_delays = sig_delays)
