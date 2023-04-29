#include "EntryBuilder.h"
#include "Channel.h"
#include "HighIncWindows.h"

#pragma warning(push) 
#pragma warning(disable: 26815) 
// there seems to be a bug in the static analysis that causes a false 
// positive on some functions returning *this (not consistent or clear or correct) 

	EntryBuilder::EntryBuilder(const wchar_t* sourceFile, const wchar_t* sourceFunctionName, int sourceLine)
		:
		Entry{
			.sourceFile_ = sourceFile,
			.sourceFunctionName_ = sourceFunctionName,
			.sourceLine_ = sourceLine,
			.timestamp_ = std::chrono::system_clock::now(),
		}
	{}
	EntryBuilder& EntryBuilder::note(std::wstring note)
	{
		note_ = std::move(note);
		return *this;
	}
	EntryBuilder& EntryBuilder::level(chil::log::Level level)
	{
		level_ = level;
		return *this;
	}
	EntryBuilder& EntryBuilder::verbose(std::wstring note)
	{
		note_ = std::move(note);
		level_ = chil::log::Level::Verbose;
		return *this;
	}
	EntryBuilder& EntryBuilder::debug(std::wstring note)
	{
		note_ = std::move(note);
		level_ = chil::log::Level::Debug;
		return *this;
	}
	EntryBuilder& EntryBuilder::info(std::wstring note)
	{
		note_ = std::move(note);
		level_ = chil::log::Level::Info;
		return *this;
	}
	EntryBuilder& EntryBuilder::warn(std::wstring note)
	{
		note_ = std::move(note);
		level_ = chil::log::Level::Warn;
		return *this;
	}
	EntryBuilder& EntryBuilder::error(std::wstring note)
	{
		note_ = std::move(note);
		level_ = chil::log::Level::Error;
		return *this;
	}
	EntryBuilder& EntryBuilder::fatal(std::wstring note)
	{
		note_ = std::move(note);
		level_ = chil::log::Level::Fatal;
		return *this;
	}
	EntryBuilder& EntryBuilder::chan(IChannel* pChan)
	{
		pDest_ = pChan;
		return *this;
	}
	EntryBuilder& EntryBuilder::trace_skip(int depth)
	{
		traceSkipDepth_ = depth;
		return *this;
	}
	EntryBuilder& EntryBuilder::no_trace()
	{
		captureTrace_ = false;
		return *this;
	}
	EntryBuilder& EntryBuilder::trace()
	{
		captureTrace_ = true;
		return *this;
	}
	EntryBuilder& EntryBuilder::no_line()
	{
		showSourceLine_ = false;
		return *this;
	}
	EntryBuilder& EntryBuilder::line()
	{
		showSourceLine_ = true;
		return *this;
	}
	EntryBuilder& EntryBuilder::hr()
	{
		hResult_ = GetLastError();
		return *this;
	}
	EntryBuilder& EntryBuilder::hr(unsigned int hr)
	{
		hResult_ = hr;
		return *this;
	}
	EntryBuilder::~EntryBuilder()
	{
		if (pDest_) {
			if (captureTrace_.value_or((int)level_ <= (int)chil::log::Level::Error)) {
				//trace_.emplace(traceSkipDepth_);
			}
			//pDest_->Submit(*this);
		}
	}

#pragma warning(pop)