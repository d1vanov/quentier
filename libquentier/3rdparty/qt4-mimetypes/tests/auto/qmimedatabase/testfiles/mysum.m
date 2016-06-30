function m=myavg(x)
% MYAVG(X) Calculates the average of the elements
n=length(x);
mysum=0;
for i=1:n
  mysum=mysum+x(i)
end
m=mysum/n;
