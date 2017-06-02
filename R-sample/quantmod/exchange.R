library('quantmod')
from <- c("AUD", "HKD", "HKD", "CAD", "JPY", "USD")
to <- c("HKD","CNH", "CNY", "HKD", "HKD", "HKD")
query <- paste0(from, to, "=X")
result <- getQuote(query)
print(result) #GMT+0 ??
