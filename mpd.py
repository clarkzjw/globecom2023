from mpegdash.parser import MPEGDASHParser


def parse_mpd(mpd_path):
    mpd = MPEGDASHParser.parse(mpd_path)

    periods = mpd.periods
    adaption_sets = periods[0].adaptation_sets

    url = dict()
    level = 1
    for adaption_set in adaption_sets:
        representations = adaption_set.representations
        for representation in representations:
            if representation.height not in url.keys():
                url[representation.height] = dict()

            temp_url = list()
            segment_template = adaption_set.segment_templates
            if len(segment_template) > 0:
                segment_template = segment_template[0]

                media_url_template = segment_template.media
                init_url_template = segment_template.initialization

                segment_timeline = segment_template.segment_timelines
                timescale = segment_template.timescale

                init_url_template = init_url_template.replace("$RepresentationID$", representation.id)
                segment_info = {"duration_seconds": 0,
                                "duration": 0,
                                "url": init_url_template}
                temp_url.append(segment_info)
                seg_no = 1

                if len(segment_timeline):
                    for segment in segment_timeline:
                        for ss in segment.Ss:
                            duration = ss.d
                            repeat = ss.r

                            if repeat is None:
                                repeat = 0

                            for i in range(repeat + 1):
                                segment_info = {"duration_seconds": (duration * 1.0 / timescale),
                                                "duration": duration,
                                                "frame_count": (duration * 1.0 / timescale) * 24,
                                                "url": media_url_template.
                                                replace("$RepresentationID$", representation.id).
                                                replace("$Number$", str(seg_no))}

                                temp_url.append(segment_info)
                                seg_no += 1

            url[representation.height][level] = temp_url
            level += 1
    return url
