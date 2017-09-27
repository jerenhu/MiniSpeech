function [result] = speechrecognition(testnum)

%disp('正在计算参考模板的参数...')
%计算参考模板的参数
for i=0:9
    fname=sprintf('./data/train/%d.wav',i);
    x=wavread(fname);
    [x1 x2]=vad(x);
    m=mfcc(x);
    m=m(x1-2:x2-4,:);
    ref(i+1).mfcc=m;
end

%计算测试模板的参数
fname=sprintf('./data/test/%d.wav',testnum);
x=wavread(fname);
[x1 x2]=vad(x);
m=mfcc(x);
m=m(x1-2:x2-4,:);
test_mfcc=m;

%进行模板匹配
dist=zeros(10,1);
for j=0:9
  dist(j+1)=dtw(test_mfcc,ref(j+1).mfcc);
  %fprintf('processing 百分之%d\n',(j+1)*10);
end

%disp('正在计算匹配结果...');
%计算匹配结果
disp(dist);
[d,j]=min(dist);
%fprintf('测试模板%d的识别结果为:%d\n',testnum,j-1);
result = j-1;

