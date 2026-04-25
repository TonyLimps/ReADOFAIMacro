#pragma once

namespace ReADOFAIMacro {
	class Range {
	public:
		const size_t start;
		const size_t end;

		Range(const size_t start, const size_t end) : start(start), end(end) {}

		bool contains(const size_t point) const {
			return start <= point && point < end;
		}

		template <typename F> void forEach(const F& action) const {
			for (size_t i = start; i < end; ++i) {
				action(i);
			}
		}
	};
}
