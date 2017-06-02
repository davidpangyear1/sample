library('quantmod')
symbol <- getSymbols("AUD/HKD",src="oanda") #"AUDHKD"
print(get(symbol))
