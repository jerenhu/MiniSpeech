function [x1,x2]=vad(x)
%%Double threshold endpoint detection algorithm

%normalized amplitude  to[-1,1]
x=double(x);
x=x/max(abs(x));

%constant setting
FrameLen=240;
FrameInc=80;

amp1=10;
amp2=2;

maxsilence=3;   %3*10ms=30ms
minlen=15;      %15*10ms=150ms
status=0;
count=0;
silence=0;

%计算过零率
tmp1 = enframe(x(1:length(x)-1),FrameLen,FrameInc);
tmp2 = enframe(x(2:length(x)),FrameLen,FrameInc);
signs = (tmp1.*tmp2)<0;
diffs = (tmp1-tmp2)>0.02;
zcr = sum(signs.*diffs,2);

%计算短时能量
amp=sum(abs(enframe(filter([1 -0.9375],1,x),FrameLen,FrameInc)),2);

%调整能量门限
amp1=min(amp1,max(amp)/4);
amp2=min(amp2,max(amp)/8);

%begin endpoint detection
x1 = 0;
x2 = 0;
for n=1:length(zcr)
  switch status
  case{0,1}         %0=静音，1=可能开始
    if amp(n)>amp1  %确信进入语音段
      x1=max(n-count-1,1);
      status=2;
      silence=0;
      count=count+1;
    elseif (amp(n)>amp2 || zcr(n)>zcr(2)) %可能处于语音段
      status=1;
      count=count+1;
    else            %静音状态
      status=0;
      count=0;
    end
  case 2,           %2=语音段
    if (amp(n)>amp(2) || zcr(n)>zcr(2)) %保持在语音段
      count=count+1;
    else            %语音将结束
      silence=silence+1;
      if silence<maxsilence     %静音还不够长，尚未结束
        count=count+1;
      elseif count<minlen       %语音长度太短，认为是噪声
        status=0;
        silence=0;
        count=0;
      else                      %语音结束
        status=3;
      end
    end
  case 3,
    break;
  end
end

count=count-silence/2;
x2= ceil(x1+count-1);
