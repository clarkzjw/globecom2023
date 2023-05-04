clear;
close all;

% linucb: 1.0
% lints: 0.2

minRTT_average_bitrate = readmatrix('../minRTT-average-bitrate.csv');
minRTT_rebuffering = readmatrix('../minRTT-rebuffering-count.csv');
RR_average_bitrate = readmatrix('../RR-average-bitrate.csv');
RR_rebuffering = readmatrix('../RR-rebuffering-count.csv');


lints_alpha = 0.2;
linucb_alpha = 1.0;

h = figure('visible','off');
legend_labels = cell(1, 4);

% average bitrate
ts_average_bitrate = readmatrix(sprintf("../LinTS-%.1f-average-bitrate.csv", lints_alpha));
ucb_average_bitrate = readmatrix(sprintf("../LinUCB-%.1f-average-bitrate.csv", linucb_alpha));

subplot(1,2,1);

legend_labels{1} = sprintf('minRTT');
[F, X] = ecdf(minRTT_average_bitrate);
plot(X, F, '-');
xlabel('Average bitrate (Kbps)');
ylabel('CDF');
hold on;

legend_labels{2} = sprintf('RR');
[F, X] = ecdf(RR_average_bitrate);
plot(X, F, '--');
ylabel('CDF');
xlabel('Average bitrate (Kbps)');
hold on;

legend_labels{3} = sprintf('LinTS');
[F, X] = ecdf(ts_average_bitrate);
p = plot(X, F, ':');
ylabel('CDF');
xlabel('Average bitrate (Kbps)');
p.Color = "red";
hold on;

legend_labels{4} = sprintf('LinUCB');
[F, X] = ecdf(ucb_average_bitrate);
plot(X, F, '-.');
ylabel('CDF');
xlabel('Average bitrate (Kbps)');
legend(legend_labels, 'Location','best');
% title('Average bitrate (Kbps)');

% rebuffering
ts_rebuffering = readmatrix(sprintf("../LinTS-%.1f-rebuffering-count.csv", lints_alpha));
ucb_rebuffering = readmatrix(sprintf("../LinUCB-%.1f-rebuffering-count.csv", linucb_alpha));

subplot(1,2,2);

legend_labels{1} = sprintf('minRTT');
[F, X] = ecdf(minRTT_rebuffering);
plot(X, F, '-');
ylabel('CDF');
xlabel('Rebuffering event count');
xlim([0, 15]);
hold on;

legend_labels{2} = sprintf('RR');
[F, X] = ecdf(RR_rebuffering);
plot(X, F, '--');
ylabel('CDF');
xlabel('Rebuffering event count');
xlim([0, 15]);
hold on;

legend_labels{3} = sprintf('LinTS');
[F, X] = ecdf(ts_rebuffering);
p = plot(X, F, ':');
ylabel('CDF');
xlabel('Rebuffering event count');
p.Color = "red";
xlim([0, 15]);
hold on;

legend_labels{4} = sprintf('LinUCB');
[F, X] = ecdf(ucb_rebuffering);
plot(X, F, '-.');
ylabel('CDF');
xlabel('Rebuffering event count');
xlim([0, 15]);
legend(legend_labels, 'Location', 'best');
% title('Rebuffering event count');

% set(h,'Units','Inches');
% pos = get(h,'Position');
% set(h,'PaperPositionMode','Auto','PaperUnits','Inches','PaperSize',[pos(3), pos(4)])
% print(h,sprintf('gcloud-lints-%.1f-linucb-%.1f.pdf', lints_alpha, linucb_alpha),'-dpdf','-bestfit','-r0')

f = gcf;

posA = get(f,'Position');
posA(3) = 1200;
posA(4) = 300;
set(f,'Position',posA);

exportgraphics(f,sprintf('mininet-lints-%.1f-linucb-%.1f.pdf', lints_alpha, linucb_alpha))

fprintf("LinTS Alpha: %.1f, LinUCB Alpha: %.1f\n", lints_alpha, linucb_alpha);
