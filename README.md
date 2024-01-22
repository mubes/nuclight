This is a simple utility that allows control of the light array on the T9Plus Mini PC.

It's based on protocol discoveries made by 'fairedummy' in Reddit r/MiniPCs Dec 2023 in 
the thread `T9 Plus N100 - How to control LED'. Kudos to him.

It is released under the standard BSD 3-clause licence, SPDX-License-Identifier: BSD-3-Clause.

Building
========

You should be able to just build the utility directly with any reasonable set of tools on Linux.
No specific libraries or setup are needed. Code was developed on Debian 12;

```
make
```

The resulting binary `nuclight` can be placed wherever is convinient.

Use
===

nuclight is used from the command line, and it's operation is pretty straightforward;

```
$nuclight -h
Usage: ./nuclight [options]
    -a, --serial-speed:  <serialSpeed> to use
    -b, --brightness:    <num> Brightness Level 1..5
    -h, --help:          This help
    -m, --mode:          <mode> 1=Rainbow  2=Breathing  3=Cycle  4=Off  5=Auto  
    -p, --serial-port:   <serialPort> to use
    -s, --speed:         <num> Speed 1..5
    -v, --verbose:       <level> Verbose mode 0(errors)..3(debug)
    -V, --version:       Print version and exit
```

So, for example, to turn the light off;
```
$nuclight -m 4
```

...or to set bright, slow, breathing;

```
$nuclight -m 2 -s 1 -b 5
```
