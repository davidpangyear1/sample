-- Variable must be in camelCase
myVar :: Int
myVar = 3

-- The bound of Int
biggestInt, smallestInt :: Int
biggestInt = maxBound
smallestInt = minBound

-- As big as your computer memory
reallyBig :: Integer
reallyBig = 2 ^ 400

-- ...And also Double, Bool, String, Char, Float
i1, i2 :: Int
i1 = -2
i2 = 4
d1, d2 :: Double
d1 = -3
d2 = 1.5
b1, b2 :: Bool
b1 = True
b2 = False
s1, s2 :: String
s1 = "Hello, "
s2 = "World!!"

main = do
    print (myVar)
    
    print (biggestInt)
    print (smallestInt)

    print (reallyBig)
    print (length (show reallyBig))

    print (s1)

    print (mod 19 3)
    print (19 `mod` 3) -- the `backticks` make a function name into an infix operator
    -- -- Error, no implicit conversion
    -- print (i1 + d1)
    -- -- Error, no integer '/', use `div` instead
    -- print (i2 / i2)
    print (i2 `div` i2)

    -- False != True
    print ((b1 && b2) /= (b1 || b2))
    -- True == True
    print ((i1 <= i2) == (s1 < s2))

    print (s2)
