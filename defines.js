module.exports = (variant) => {
	return {
		// The --log-level flag can further raise this, but this eliminates logs below this level at compile-time
		SPDLOG_ACTIVE_LEVEL: 'SPDLOG_LEVEL_TRACE',
		// SPDLOG_ACTIVE_LEVEL: 'SPDLOG_LEVEL_DEBUG',
		// SPDLOG_ACTIVE_LEVEL: 'SPDLOG_LEVEL_INFO',
		// SPDLOG_ACTIVE_LEVEL: 'SPDLOG_LEVEL_WARN',
		// SPDLOG_ACTIVE_LEVEL: 'SPDLOG_LEVEL_ERROR',
		// SPDLOG_ACTIVE_LEVEL: 'SPDLOG_LEVEL_CRITICAL',
		// SPDLOG_ACTIVE_LEVEL: 'SPDLOG_LEVEL_OFF',
	};
};
