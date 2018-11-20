NUM_BINS = 50
sumB = 0
binFreq = 44
genFrequencyBinsHorizontal = []
genFrequencyLabelsHorizontal = []

for i in range(0,120):

	genFrequencyBinsHorizontal.append(60./NUM_BINS*0.7964*pow(2.71828,0.0583*(i + 1)*(60./NUM_BINS)))
print(genFrequencyBinsHorizontal)

sum  = 0
for i in genFrequencyBinsHorizontal:
	sum = sum + i
print(sum)
for x in range(0,NUM_BINS):
    genFrequencyLabelsHorizontal.append(genFrequencyBinsHorizontal[x]*binFreq + sumB);
    sumB = genFrequencyLabelsHorizontal[x];
print(genFrequencyLabelsHorizontal)
print((0.0075*45+80)/80)
print((0.0075*4000+80)/80)