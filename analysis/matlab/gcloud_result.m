clear;
close all;

lints_alpha_arr = [0.2, 0.4, 0.6, 0.8, 1.0];
linucb_alpha_arr = [0.2, 0.4, 0.6, 0.8, 1.0];

% linucb: 1.0
% lints: 0.2

minRTT_average_bitrate = readmatrix('../gcloud-minrtt-average-bitrate.csv');
minRTT_rebuffering = readmatrix('../gcloud-minrtt-rebuffering-count.csv');
RR_average_bitrate = readmatrix('../gcloud-rr-average-bitrate.csv');
RR_rebuffering = readmatrix('../gcloud-rr-rebuffering-count.csv');


for i = 1:length(lints_alpha_arr)
    for j = 1:length(linucb_alpha_arr)
        lints_alpha = lints_alpha_arr(i);
        linucb_alpha = linucb_alpha_arr(j);

        h = figure('visible','off');
        legend_labels = cell(1, 4);

        % average bitrate
        ts_average_bitrate = readmatrix(sprintf("../gcloud_lints-%.1f-average-bitrate.csv", lints_alpha));
        ucb_average_bitrate = readmatrix(sprintf("../gcloud_linucb-%.1f-average-bitrate.csv", linucb_alpha));

        subplot(2,1,1);

        legend_labels{1} = sprintf('minRTT');
        [F, X] = ecdf(minRTT_average_bitrate);
        plot(X, F, '-');
        hold on;

        legend_labels{2} = sprintf('RR');
        [F, X] = ecdf(RR_average_bitrate);
        plot(X, F, '--');
        hold on;

        legend_labels{3} = sprintf('LinTS Alpha: %.1f', lints_alpha);
        [F, X] = ecdf(ts_average_bitrate);
        plot(X, F, ':');
        hold on;

        legend_labels{4} = sprintf('LinUCB Alpha: %.1f', linucb_alpha);
        [F, X] = ecdf(ucb_average_bitrate);
        plot(X, F, '-.');
        legend(legend_labels, 'Location','northeastoutside');

        % rebuffering
        ts_rebuffering = readmatrix(sprintf("../gcloud_lints-%.1f-rebuffering-count.csv", lints_alpha));
        ucb_rebuffering = readmatrix(sprintf("../gcloud_linucb-%.1f-rebuffering-count.csv", linucb_alpha));

        subplot(2,1,2);
        legend_labels{1} = sprintf('minRTT');
        [F, X] = ecdf(minRTT_rebuffering);
        plot(X, F, '-');
        xlim([0, 100]);
        hold on;

        legend_labels{2} = sprintf('RR');
        [F, X] = ecdf(RR_rebuffering);
        plot(X, F, '--');
        xlim([0, 100]);
        hold on;

        legend_labels{3} = sprintf('LinTS Alpha: %.1f', lints_alpha);
        [F, X] = ecdf(ts_rebuffering);
        plot(X, F, ':');
        xlim([0, 100]);
        hold on;

        legend_labels{4} = sprintf('LinUCB Alpha: %.1f', linucb_alpha);
        [F, X] = ecdf(ucb_rebuffering);
        plot(X, F, '-.');
        xlim([0, 100]);
        legend(legend_labels, 'Location','northeastoutside');


        % set(h,'Units','Inches');
        % pos = get(h,'Position');
        % set(h,'PaperPositionMode','Auto','PaperUnits','Inches','PaperSize',[pos(3), pos(4)])
        print(h,sprintf('gcloud-lints-%.1f-linucb-%.1f.png', lints_alpha, linucb_alpha),'-dpng','-r0')

        fprintf("LinTS Alpha: %.1f, LinUCB Alpha: %.1f\n", lints_alpha, linucb_alpha);
    end
end

for j = 1:length(linucb_alpha_arr)
    for i = 1:length(lints_alpha_arr)
        linucb_alpha = linucb_alpha_arr(j);
        lints_alpha = lints_alpha_arr(i);

        h = figure('visible','off');
        legend_labels = cell(1, 4);

        % average bitrate
        ts_average_bitrate = readmatrix(sprintf("../gcloud_lints-%.1f-average-bitrate.csv", lints_alpha));
        ucb_average_bitrate = readmatrix(sprintf("../gcloud_linucb-%.1f-average-bitrate.csv", linucb_alpha));

        subplot(2,1,1);

        legend_labels{1} = sprintf('minRTT');
        [F, X] = ecdf(minRTT_average_bitrate);
        plot(X, F, '-');
        hold on;

        legend_labels{2} = sprintf('RR');
        [F, X] = ecdf(RR_average_bitrate);
        plot(X, F, '--');
        hold on;

        legend_labels{3} = sprintf('LinTS Alpha: %.1f', lints_alpha);
        [F, X] = ecdf(ts_average_bitrate);
        plot(X, F, ':');
        hold on;

        legend_labels{4} = sprintf('LinUCB Alpha: %.1f', linucb_alpha);
        [F, X] = ecdf(ucb_average_bitrate);
        plot(X, F, '-.');
        legend(legend_labels, 'Location','northeastoutside');
       
        % rebuffering
        ts_rebuffering = readmatrix(sprintf("../gcloud_lints-%.1f-rebuffering-count.csv", lints_alpha));
        ucb_rebuffering = readmatrix(sprintf("../gcloud_linucb-%.1f-rebuffering-count.csv", linucb_alpha));

        subplot(2,1,2);
        legend_labels{1} = sprintf('minRTT');
        [F, X] = ecdf(minRTT_rebuffering);
        plot(X, F, '-');
        xlim([0, 100]);
        hold on;

        legend_labels{2} = sprintf('RR');
        [F, X] = ecdf(RR_rebuffering);
        plot(X, F, '--');
        xlim([0, 100]);
        hold on;

        legend_labels{3} = sprintf('LinTS Alpha: %.1f', lints_alpha);
        [F, X] = ecdf(ts_rebuffering);
        plot(X, F, ':');
        xlim([0, 100]);
        hold on;

        legend_labels{4} = sprintf('LinUCB Alpha: %.1f', linucb_alpha);
        [F, X] = ecdf(ucb_rebuffering);
        plot(X, F, '-.');
        xlim([0, 100]);
        legend(legend_labels, 'Location','northeastoutside');


        % set(h,'Units','Inches');
        % pos = get(h,'Position');
        % set(h,'PaperPositionMode','Auto','PaperUnits','Inches','PaperSize',[pos(3), pos(4)])
        print(h,sprintf('gcloud-linucb-%.1f-lints-%.1f.png', linucb_alpha, lints_alpha),'-dpng','-r0')

        fprintf("LinTS Alpha: %.1f, LinUCB Alpha: %.1f\n", lints_alpha, linucb_alpha);
    end
end