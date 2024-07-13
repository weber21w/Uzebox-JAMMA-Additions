dconvert 2017 Lee Weber released under GPL 3

dconvert is designed to use a configuration file which specifies 1 or more transformations from input file(s) to output file(s).

Perhaps it's most common use is to create binary SD data, from multiple binary files, multiple C include files, or some mixture.
This can be integrated into a project's makefile so that a complex layout can be rapidly reproduced when changes are made. The uses
are many once you understand the capabilities.

dconvert is used as: "dconvert config-file.cfg", see demos/SpiRamMusicDemo for an example of 1 possible usage.

The configuration file passed to the program specifies the transformation details via comma separated values. The first line of this file sets a number of things for the remaining entries:
parameter 1: 0 = input file is C array, 1 = input file is binary
parameter 2: 0 = output file is C array, 1 = output file is binary
parameter 3: X = offset to start writing directory to converted objects(32 bit), -1 = don't write a directory
parameter 4: X = offset to start writing output data(meaningless for C output)
parameter 5: 0 = no sector padding, 1 = pad out to even multiple of 512
parameter 6: X = minimum file size(padded with 0 output is output start+size is less than)
parameter 7: name of output file

After the first line, any number of entry lines may be specified. If you wish to comment a line out, start it with a '#' character.

The format of the entry lines is:
parameter 1: name of input file
parameter 2: number of arrays to skip first(meaningless for binary input)
parameter 3: number of bytes to skip(in selected array) before processing data(meaningless for binary output)
parameter 4: 0 = raw transform, 1 = Patch Transform, >1 = ..future use
parameter 5: name of output array(meaningless for binary output)
