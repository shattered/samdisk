// KryoFlux real device wrapper

#include "SAMdisk.h"
#include "DemandDisk.h"
#include "BitstreamDecoder.h"
#include "KryoFlux.h"

class KFDevDisk final : public DemandDisk
{
public:
	explicit KFDevDisk (std::unique_ptr<KryoFlux> kryoflux)
		: m_kryoflux(std::move(kryoflux))
	{
		try
		{
			m_kryoflux->SetMinTrack(0);
			m_kryoflux->SetMaxTrack(83 - 1);
			m_kryoflux->SelectDevice(0);
			m_kryoflux->Seek(0);
		}
		catch (...)
		{
			throw util::exception("failed to initialise KryoFlux device");
		}
	}

	~KFDevDisk ()
	{
		m_kryoflux->EnableMotor(0);
	}

protected:
	TrackData load (const CylHead &cylhead, bool first_read) override
	{
		FluxData flux_revs;
		auto revs = first_read ? FIRST_READ_REVS : REMAIN_READ_REVS;

		m_kryoflux->EnableMotor(1);

		m_kryoflux->Seek(cylhead.cyl);
		m_kryoflux->SelectSide(cylhead.head);

		std::vector<std::string> warnings;
		m_kryoflux->ReadFlux(revs + 1, flux_revs, warnings);
		for (auto &w : warnings)
			Message(msgWarning, "%s on %s", w.c_str(), CH(cylhead.cyl, cylhead.head));

		return TrackData(cylhead, std::move(flux_revs));
	}

	bool preload (const Range &/*range*/) override
	{
		return false;
	}

private:
	std::unique_ptr<KryoFlux> m_kryoflux;
};


bool ReadKFDev (const std::string &/*path*/, std::shared_ptr<Disk> &disk)
{
	// ToDo: use path to select from multiple devices?

	auto kryoflux = KryoFlux::Open();
	if (!kryoflux)
		throw util::exception("failed to open KryoFlux device");

	auto kf_dev_disk = std::make_shared<KFDevDisk>(std::move(kryoflux));
	kf_dev_disk->extend(CylHead(83 - 1, 2 - 1));

	kf_dev_disk->strType = "KryoFlux";
	disk = kf_dev_disk;

	return true;
}
