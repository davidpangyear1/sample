#data
x=c(22,28,23,22,29,25,24,29)
y=c( 7,10, 5, 6,11, 6, 5,12)
n=length(x)

################Regression################

#sum of squares
SSxx=sum((x-mean(x))*(x-mean(x)))
SSxy=sum((x-mean(x))*(y-mean(y)))
SSyy=sum((y-mean(y))*(y-mean(y)))
print(c(SSxx,SSxy,SSyy))

#parameters (beta's)
beta1=SSxy/SSxx
beta0=mean(y)-beta1*mean(x)
yhead=beta0+beta1*x

#sample variance
SSE=sum((y-yhead)*(y-yhead))
s=sqrt(SSE/(n-2))
print(s)

################Prediction################
#apply formula on particular x=26
xp=26
t=qt(.975, df=n-2) #alpha=.05, confidence=95%
yphead=beta0+beta1*xp

#Q1, Expectation at x=26
stderr=s*sqrt(1/n+((xp-mean(x))^2/SSxx))
I=c(yphead-t*stderr,yphead+t*stderr)
print(I)

#Q2, Prediction at x=26, (stderr needs to add 1 under sqrt)
stderr=s*sqrt(1+1/n+((xp-mean(x))^2/SSxx))
I=c(yphead-t*stderr,yphead+t*stderr)
print(I)

################Use R functions directly################
#Check answer at x=26
fit = lm(y~x)
xpdata = data.frame(x=26)

#Q1
predict(fit,xpdata,interval="confidence")

#Q2
predict(fit,xpdata,interval="predict")
