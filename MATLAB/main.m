clear all;

result = zeros(10,1);
%识别0-9
for i = 0:9
    fprintf('正在识别数字%d...',i);
    result(i+1) = speechrecognition(i);
    fprintf('数字%d的识别结果是%d\n',i,result(i+1));
end
