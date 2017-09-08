function rawDataVisulization(filePath, figureId)
% this function visualizes raw data produced by moses
% e.g. (lats.bin); input arguments are file path 
% and id for which figure to display (default to 1)
    if nargin < 1
       error('Please specify a file to be visualized') 
    elseif nargin < 2
       disp('No figureId is specified, using 1 by default')
        figureId = 1;
    end
    data = importdata(filePath);
    svcTime = data(:,2);
    ltcTime = data(:,3);
    clear data;
    nData = numel(svcTime);
    disp([num2str(nData), ' lines of data are read'])
    idVec = 1:1:nData;
    
    figure(figureId);clf;
    plot(idVec,svcTime,'ro',idVec,ltcTime,'bx')
    xlabel('response received')
    ylabel('time (ns)' )
    legend('service time', 'latency time')
end