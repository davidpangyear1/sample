n<-25000

#Uniform Distribution
?runif
u<-runif(n)

#Normal Distribution
?rnorm
x<-rnorm(n,mean,variance)

#Gamma Distribution
?rgamma
x<-rgamma(n,alpha,scale=beta)
x<-rgamma(n,alpha,rate=lambda)
