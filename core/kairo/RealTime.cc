// openMSX's RealTime (RealTime.hh) for KairoMSX, where the host runs the machine one frame at a
// time and paces the frames to the audio device. openMSX's own RealTime sleeps to hold emulated
// time to the wall clock and skips drawing frames when it falls behind; here emulated time is
// the clock, so it never sleeps, there is always time left to draw a frame, and every frame is
// drawn. Emulation therefore does not depend on how fast the host runs.

#include "RealTime.hh"

#include "BooleanSetting.hh"
#include "Event.hh"
#include "EventDelay.hh"
#include "EventDistributor.hh"
#include "GlobalSettings.hh"
#include "MSXMotherBoard.hh"
#include "Reactor.hh"
#include "ThrottleManager.hh"

namespace openmsx {

// How often queued host events are passed on to the machine, as openMSX's RealTime does.
const double SYNC_INTERVAL = 0.08;  // s

RealTime::RealTime(
		MSXMotherBoard& motherBoard_, GlobalSettings& globalSettings,
		EventDelay& eventDelay_)
	: Schedulable(motherBoard_.getScheduler())
	, motherBoard(motherBoard_)
	, eventDistributor(motherBoard.getReactor().getEventDistributor())
	, eventDelay(eventDelay_)
	, speedManager   (globalSettings.getSpeedManager())
	, throttleManager(globalSettings.getThrottleManager())
	, pauseSetting   (globalSettings.getPauseSetting())
	, powerSetting   (globalSettings.getPowerSetting())
	, idealRealTime(0)
	, sleepAdjust(0.0)
{
	speedManager.attach(*this);
	throttleManager.attach(*this);
	pauseSetting.attach(*this);
	powerSetting.attach(*this);

	resync();

	for (auto type : {EventType::FINISH_FRAME, EventType::FRAME_DRAWN}) {
		eventDistributor.registerEventListener(type, *this);
	}
}

RealTime::~RealTime()
{
	for (auto type : {EventType::FRAME_DRAWN, EventType::FINISH_FRAME}) {
		eventDistributor.unregisterEventListener(type, *this);
	}
	powerSetting.detach(*this);
	pauseSetting.detach(*this);
	throttleManager.detach(*this);
	speedManager.detach(*this);
}

double RealTime::getRealDuration(EmuTime time1, EmuTime time2) const
{
	return (time2 - time1).toDouble() / speedManager.getSpeed();
}

EmuDuration RealTime::getEmuDuration(double realDur) const
{
	return EmuDuration::sec(realDur * speedManager.getSpeed());
}

bool RealTime::timeLeft(uint64_t /*us*/, EmuTime /*time*/) const
{
	return true;
}

void RealTime::sync(EmuTime time, bool allowSleep)
{
	if (allowSleep) {
		removeSyncPoint();
	}
	internalSync(time, allowSleep);
	if (allowSleep) {
		setSyncPoint(time + getEmuDuration(SYNC_INTERVAL));
	}
}

void RealTime::internalSync(EmuTime time, bool allowSleep)
{
	if (allowSleep) {
		eventDelay.sync(time);
	}
	emuTime = time;
}

void RealTime::executeUntil(EmuTime time)
{
	internalSync(time, true);
	setSyncPoint(time + getEmuDuration(SYNC_INTERVAL));
}

bool RealTime::signalEvent(const Event& event)
{
	if (!motherBoard.isActive() || !enabled) {
		return false;
	}
	visit(overloaded{
		[&](const FinishFrameEvent& ffe) {
			if (!ffe.needRender()) {
				sync(getCurrentTime(), false);
			}
		},
		[&](const FrameDrawnEvent&) {
			sync(getCurrentTime(), true);
		},
		[&](const EventBase /*e*/) {
		}
	}, event);
	return false;
}

void RealTime::update(const Setting& /*setting*/) noexcept
{
	resync();
}

void RealTime::update(const SpeedManager& /*speedManager*/) noexcept
{
	resync();
}

void RealTime::update(const ThrottleManager& /*throttleManager*/) noexcept
{
	resync();
}

void RealTime::resync()
{
	if (!enabled) return;

	removeSyncPoint();
	emuTime = getCurrentTime();
	setSyncPoint(emuTime + getEmuDuration(SYNC_INTERVAL));
}

void RealTime::enable()
{
	enabled = true;
	resync();
}

void RealTime::disable()
{
	enabled = false;
	removeSyncPoint();
}

} // namespace openmsx
