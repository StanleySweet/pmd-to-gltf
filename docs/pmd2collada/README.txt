PMD to Collada converter, pmd2collada.py
Converts 3d graphic files in the PMD format to the COLLADA XML-based
format
(C) 2010 by Matthew Minton, v1.0
(C) 2011 modified by JeffG, v2.0
(C) 2012 modified by Ben Brian, v3.0
 
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

Usage (long version): Edit pmd2collada.py with a new PMD file path and run or,

Usage (short version): Import pmd2collada into your own script that lists 
the PMD files you want to convert. Feed a new instance of 
pmd2collada.PMDtoCollada(<filename>) the full path and watch it go. PMDtoCollada 
will convert the PMD files to DAE and put them in the same directory
as the source file...OR

Usage (short, short version): Run pmd2colladaLauncher.py, give it the path to the
top level of your 0AD directory (or whereever your /binaries/data/mods/public/art 
directory is located, if you want it to run faster) and optionally the location
of skeletons.xml (to support skeletal models), and let slip the dogs of war.

Version History:

v1.0: Initial release, 8/19/2010
v2.0: Fixed crashes and tweaked things, 05/2011
v3.0: Major fixes, added support for bones and prop points, 01/2012
