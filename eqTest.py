NUM_BINS = 120
genFrequencyBinsHorizontal = []
for i in range(0,120):

	genFrequencyBinsHorizontal.append(60./NUM_BINS*0.7964*pow(2.71828,0.0583*(i + 1)*(60./NUM_BINS)))
print(genFrequencyBinsHorizontal)

sum  = 0
for i in genFrequencyBinsHorizontal:
	sum = sum + i
print(sum)