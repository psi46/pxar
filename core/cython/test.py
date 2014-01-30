import PyPxarCore
from PyPxarCore import Pixel
p = Pixel()
p.decode(12345)
print p.column
p2 = Pixel()
print p2.column
from PyPxarCore import PyPxarCore
api = PyPxarCore()
print api.getVersion()

