-- sumtorial
sumtorial :: Integer -> Integer -- Not necessary but recommended
sumtorial 0 = 0
sumtorial n = n + sumtorial (n-1)

-- hailstone, return n/2 or 3n+1
hailstone :: Integer -> Integer
hailstone n
  | n `mod` 2 == 0 = n `div` 2
  | otherwise      = 3*n + 1

-- try hailstone path, return count
try_hailstone :: (Integer,Integer) -> Integer
try_hailstone (c,n)
  | n == 1    = c
  | otherwise = try_hailstone (c+1, hailstone(n))

main = do
    sumtorial(5)
