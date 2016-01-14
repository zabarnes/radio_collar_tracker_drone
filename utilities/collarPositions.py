#!/usr/bin/env python
# This program takes in the location of the balloon test rig and generates a
# sequence of positions that we should use for testing for the collar.

import argparse
import utm
import math

parser = argparse.ArgumentParser(description = "Balloon Test Position Generator")
parser.add_argument('zone_num', help='UTM Zone Number')
parser.add_argument('zone_letter', help='UTM Zone Letter')
parser.add_argument('easting', help='UTM Easting')
parser.add_argument('northing', help='UTM Northing')
parser.add_argument('heading', help='Primary Heading')
parser.add_argument('-r', '--numRadials', type = int, default = 8, help = 'Number of radials', required = False)
parser.add_argument('-n', '--numSteps', type = int, default = 5, help = 'Number of steps', required = False)
parser.add_argument('-s', '--stepSize', type = float, default = 20, help = 'Size of steps', required = False)


args = parser.parse_args()

zone_num = int(args.zone_num)
zone_letter = args.zone_letter
easting = int(args.easting)
northing = int(args.northing)
hdg = float(args.heading)

numRadials = args.numRadials
numSteps = args.numSteps
stepSize = args.stepSize

radialStep = 360.0 / numRadials

position_counter = 1 + numRadials * numSteps
print("%d positions total" % position_counter)


print('%d %s %d %d' % (zone_num, zone_letter, easting, northing))

for phi_i in xrange(0, numRadials):
	for r_i in xrange(1, numSteps + 1):
		phi = (phi_i * radialStep + hdg) % 360
		r = r_i * stepSize
		easting_dx = math.cos(phi) * r
		northing_dy = math.sin(phi) * r
		print('%d %s %d %d' % (zone_num, zone_letter, easting + easting_dx, northing + northing_dy))
		position_counter += 1
