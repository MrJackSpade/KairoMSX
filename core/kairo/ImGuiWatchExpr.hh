// The debugger's watch expressions (openMSX's imgui/ImGuiWatchExpr.hh) for KairoMSX: the list
// the "debug watch_expr" command manages, without its ImGui window. WatchExpr is openMSX's own.

#ifndef KAIRO_IMGUI_WATCH_EXPR_HH
#define KAIRO_IMGUI_WATCH_EXPR_HH

// Code that includes openMSX's header gets the manager through it.
#include "ImGuiManager.hh"
#include "TclObject.hh"

#include "strCat.hh"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace openmsx {

class ImGuiWatchExpr
{
public:
	struct WatchExpr {
		static constexpr std::string_view prefix = "we#";

		WatchExpr() : id(++lastId) {}
		WatchExpr(std::string description_, std::string exprStr_, TclObject format_)
			: id(++lastId)
			, description(std::move(description_))
			, exprStr(std::move(exprStr_)) // leave 'expression' empty
			, format(std::move(format_)) {}

		[[nodiscard]] unsigned getId() const { return id; }
		[[nodiscard]] std::string getIdStr() const { return strCat(prefix, id); }
		[[nodiscard]] const auto& getDescription() const { return description; }
		[[nodiscard]] const auto& getExpression()  const { return exprStr; }
		[[nodiscard]] const auto& getFormat()      const { return format; }
		void setDescription(const TclObject& d) { description = d.getString(); }
		void setExpression (const TclObject& e) { exprStr = e.getString(); expression.reset(); }
		void setFormat     (const TclObject& f) { format = f.getString(); }

		unsigned id = 0;
		std::string description;
		std::string exprStr;
		std::optional<TclObject> expression; // cache, generate from 'expression'
		TclObject format;

		static inline unsigned lastId = 0;
	};

	[[nodiscard]] auto& getWatchExprs() { return watches; }

private:
	std::vector<WatchExpr> watches;
};

} // namespace openmsx

#endif // KAIRO_IMGUI_WATCH_EXPR_HH
