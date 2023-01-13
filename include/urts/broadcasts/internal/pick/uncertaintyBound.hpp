#ifndef URTS_BROADCASTS_INTERNAL_PICK_UNCERTAINTY_BOUND_HPP
#define URTS_BROADCASTS_INTERNAL_PICK_UNCERTAINTY_BOUND_HPP
#include <chrono>
#include <memory>
namespace URTS::Broadcasts::Internal::Pick
{
/// @class UncertaintyBound "uncertaintyBound.hpp" "urts/broadcasts/internal/pick/uncertaintyBound.hpp"
/// @brief Defines an uncertainty bound for a pick.  In this parameterization
///        an uncertainty bound corresponds to a confidence percentile and
///        a perturbation to add to the pick to reach that bound.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class UncertaintyBound
{
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    UncertaintyBound();
    /// @brief Coyp constructor.
    /// @param[in] bound  The uncertainty bound from which to initialize
    ///                   this class.
    UncertaintyBound(const UncertaintyBound &bound);
    /// @brief Move constructor.
    /// @param[in,out] bound  The uncertainty bound from which to initialize
    ///                       this class.  On exit, bound's behavior is
    ///                       undefined.
    UncertaintyBound(UncertaintyBound &&bound) noexcept;
    /// @}

    /// @brief Sets the perturbation to add to this pick to move it to the
    ///        corresponding bound.  This should be negative for percentiles
    ///        less than 50 and positive for percentiles greater than 0.
    void setPerturbation(const std::chrono::microseconds &perturbation) noexcept;
    /// @result The perturbation to add to the pick to move it to this
    ///         percentile.  By default this is 0.
    [[nodiscard]] std::chrono::microseconds getPerturbation() const noexcept;
 
    /// @brief Sets the uncertainty percentile.
    /// @param[in] percentile  The uncertainty percentile - e.g., 2.5 or 97.5.
    /// @throws std::invalid_argument if this is not in range [0,100].
    void setPercentile(double percentile);
    /// @result The percentile.  By default this is 50. 
    [[nodiscard]] double getPercentile() const noexcept;

    /// @name Operators
    /// @{

    /// @brief Copy assignment operator.
    /// @param[in] bound  The bound to copy to this.
    /// @result A deep copy of the bound.
    UncertaintyBound &operator=(const UncertaintyBound &bound);
    /// @brief Move assignment operator.
    /// @param[in,out] bound  The bound whose memory will be moved to this.
    ///                       On exit, bound's behavior is undefined.
    /// @result The memory from the bound moved to this.
    UncertaintyBound &operator=(UncertaintyBound &&bound) noexcept;
    /// @}

    /// @name Destructors
    /// @{

    /// @brief Resets the class.
    void clear() noexcept;
    /// @brief Destructor.
    ~UncertaintyBound();
    /// @}
private:
    class UncertaintyBoundImpl;
    std::unique_ptr<UncertaintyBoundImpl> pImpl;
};
}
#endif
