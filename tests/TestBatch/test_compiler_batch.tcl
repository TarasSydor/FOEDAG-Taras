#Copyright 2021 The Foedag team

#GPL License

#Copyright (c) 2021 The Open-Source FPGA Foundation

#This program is free software: you can redistribute it and/or modify
#it under the terms of the GNU General Public License as published by
#the Free Software Foundation, either version 3 of the License, or
#(at your option) any later version.

#This program is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#GNU General Public License for more details.

#You should have received a copy of the GNU General Public License
#along with this program.  If not, see <http://www.gnu.org/licenses/>.

batch {
  pin_loc_assign_method random
  synth
  packing
  globp
  
  pin_loc_assign_method in_define_order
  synth
  packing
  globp

}

after 22000 {set CONT 0}
set CONT 1 
while {$CONT} {
  set a 0
  after 100 set a 1
  vwait a
}
