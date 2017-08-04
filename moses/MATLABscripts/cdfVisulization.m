function cdfVisulization(filePath, figureId)
% this function visualizes cdf files produced by parselatsbin.py
%input arguments are file path and id for which figure to display (default to 1)
    if nargin < 1
       error('Please specify a file to be visualized') 
    elseif nargin < 2
       disp('No figureId is specified, using 1 by default')
        figureId = 1;
    end
    data = importdata(filePath);
    latency = data(:,1);
    percentile = data(:,2);
    clear data;
    
    figure(figureId);clf;
    plot(percentile,latency,'k-')
    xlabel('percentile')
    ylabel('time (ms)' )
end