h = figure;



x = readmatrix('latency.txt');
x(x < 0 | x > 1000) = [];

[F, X] = ecdf(x);
ax = plot(X, F, 'LineWidth',4);
xlim([0 200])
ylabel("CDF")
set(h,'Units','Inches');
set(gca,'fontsize', 24) 
pos = get(h,'Position');
pos(3) = 5.5;
pos(4) = 5;
set(h,'Position',pos);

saveas(h,'ping.svg');

x = readmatrix('speed.txt');
[F, X] = ecdf(x);
ax = plot(X, F, 'LineWidth',4);
ylabel("CDF")
set(h,'Units','Inches');
set(gca,'fontsize', 24) 
pos = get(h,'Position');
pos(3) = 5.5;
pos(4) = 5;
set(h,'Position',pos);

saveas(h,'speed.svg');