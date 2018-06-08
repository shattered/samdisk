// Unknown flux dump format via http://forum.agatcomp.ru//viewtopic.php?id=158
//
// This is the output of a sampler running on 100 MHz clock. File is just stream of 16-bit words,
// little endian. So each sample is with a resolution of 10 ns. No further structure.
//
// Short Apple II program "scans" 36 tracks, including the quarter tracks in-between. Total should be
// 141. The board just captures (passively) the RD signal from the floppy drive. Each track is
// scanned about 8 times. That's why the huge size.
//
// Each sample denotes the width of a pulse (the time period between two pulses), so in a way:
//
// - value about 400 (4 us) is bitstream "1"
// - value about 800 (8 us) is bitstream "01"
// - value about 1200 (12 us) is bitstream "001"
//
// Seems the images have been uploaded. This took long enough .. but still faster than sending a
// flash drive to Russia. :)
//
// The one in the top directory called "dii.merlin_dii" has the source of the small A2 scanning
// program and is standard 16-sector, so you'd better start with it. Extracting the sectors is
// relatively easy.

#include "SAMdisk.h"
#include "DemandDisk.h"
#include "BitstreamDecoder.h"


class DIIDisk final : public DemandDisk
{
public:
	void add_track_data (const CylHead &cylhead, std::vector<uint16_t> &&trackdata)
	{
		m_data[cylhead] = std::move(trackdata);
		extend(cylhead);
	}

protected:
	TrackData load (const CylHead &cylhead, bool /*first_read*/) override
	{
		const auto &data = m_data[cylhead];

		if (data.empty())
			return TrackData(cylhead);

		FluxData flux_revs;
		std::vector<uint32_t> flux_times;
		flux_times.reserve(data.size());

		uint64_t total_time = 0, total_flux = 0;
		for (auto time : data)
		{
			flux_times.push_back(time * 10);
			total_time += time * 10;
			total_flux ++;

//			if (total_time >= 200000000U * 8U)
//				break;
		}

//		util::cout << util::fmt ("total_time %d total_flux %d\n", total_time, total_flux);

		if (!flux_times.empty())
			flux_revs.push_back(std::move(flux_times));

// causes random crashes
//		m_data.erase(cylhead);

		return TrackData(cylhead, std::move(flux_revs));
	}

private:
	std::map<CylHead, std::vector<uint16_t>> m_data {};
};


bool ReadDII (MemFile &file, std::shared_ptr<Disk> &disk)
{
	if (!IsFileExt(file.name(), "dii") && util::lowercase(file.name().substr(0, 3)) != "dii")
		return false;

	auto dii_disk = std::make_shared<DIIDisk>();

	if (opt.debug)
	util::cout << util::fmt ("force %d quarter %d\n", opt.force, opt.quarter);

#if 0
	for (int cyl=0; cyl < 141; cyl++)
	{
		int track = (cyl + opt.quarter) / 4;
		if (track < 0 || track > 34) continue;

		int quarter = opt.force ? (-opt.quarter) : opt.quarter;

		CylHead cylhead(track, 0);
		std::vector<uint16_t> dii_data(file.size() / 2 / 141);

		if (!file.read(dii_data))
			throw util::exception("short file reading data");

		if (opt.debug)
		util::cout << util::fmt ("cyl %3d track %2d dii_data size %d file size %d\n", cyl, track, dii_data.size(), file.size());

//		if ((cyl % 4) == (opt.force ? (-opt.quarter) : opt.quarter))
		if ((cyl % 4) == quarter)
		{
			dii_disk->add_track_data(cylhead, std::move(dii_data));
		}
	}
#else
	for (int cyl=0; cyl < 141; cyl++)
	{
		CylHead cylhead(cyl, 0);
		std::vector<uint16_t> dii_data(file.size() / 2 / 141);

		if (!file.read(dii_data))
			throw util::exception("short file reading data");

		if (opt.debug)
		util::cout << util::fmt ("cyl %3d dii_data size %d file size %d\n", cyl, dii_data.size(), file.size());

		dii_disk->add_track_data(cylhead, std::move(dii_data));
	}
#endif

	dii_disk->strType = "DII";
	disk = dii_disk;

	return true;
}
