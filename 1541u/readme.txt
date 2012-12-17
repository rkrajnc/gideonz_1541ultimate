/*
 *   1541 Ultimate - The Storage Solution for your Commodore computer.
 *   Copyright (C) 2009  Gideon Zweijtzer - Gideon's Logic Architectures
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Abstract:
 *   Directory structure notes
 */

General rule; code and locally includable files go in 'src' (source-)
directories, includable files exporting prototypes and definitions go
in 'inc' (include-) directories. Make- files and related files reside
in the 'make' directory, which is placed on the same level as src and
inc folders in case something must be built.

Application 
	This will contain whatever application created (disk
	feeder/sid player.etc)

Filesystem 
	This will contain code for whatever file system there may
	be supported.

Filetype 
	This will contain code assisting in decoding files (d64, t64
	...etc)

Storage 
	This will contain code for taking advantage of the various
	storage the card offers.

UI 
	This will contain code for easy user interfacing via VIC / UART
	(cli, or gui- like components)

IO 
	This will contain code providing low level IO functionality (UART,
	LED, Audio.etc)

.. This list will grow as work progresses!!!

NOTE:
	It might not be needed to have subfolders differentiating the source 
	files. The source files themselves might be sufficient for that.
	For instance; file types does not contain more folders but only src 
	and inc.
	