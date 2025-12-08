#pragma once
#include <stdexcept>

template <class IntType = int>
class UniformDistribution
{
public:
    using result_type = IntType;

    static_assert(std::is_integral<IntType>::value, "Integral type required");

    struct param_type {
        result_type a;
        result_type b;

        param_type(result_type a = 0,
                   result_type b = std::numeric_limits<result_type>::max())
            : a(a)
            , b(b)
        {
            if (a > b) {
                throw std::invalid_argument("UniformDistribution: a > b");
            }
        }

        result_type min() const { return a; }
        result_type max() const { return b; }

        bool operator==(const param_type& other) const {
            return a == other.a && b == other.b;
        }

        bool operator!=(const param_type& other) const {
            return !(*this == other);
        }
    };

    UniformDistribution(result_type a = 0,
                        result_type b = std::numeric_limits<result_type>::max())
        : params_(a, b) {}

    UniformDistribution(const param_type& p) : params_(p) {}

    void reset() {}

    param_type param() const { return params_; }
    void param(const param_type& p) { params_ = p; }

    result_type min() const { return params_.min(); }
    result_type max() const { return params_.max(); }

    template<class URBG>
    result_type operator()(URBG& g) {
        return generate(g, params_);
    }

    template<class URBG>
    result_type operator()(URBG& g, const param_type& p) {
        return generate(g, p);
    }

private:
    param_type params_;

    template<class URBG>
    static result_type generate(URBG& g, const param_type& p) {
        //using URBG_result_type = typename URBG::result_type;
        using unsigned_type = typename std::make_unsigned<result_type>::type;

        unsigned_type range = static_cast<unsigned_type>(p.max()) - static_cast<unsigned_type>(p.min()) + 1;

        unsigned_type max = static_cast<unsigned_type>(URBG::max() - URBG::min());
        unsigned_type buckets = max / range;
        unsigned_type limit = buckets * range;

        unsigned_type r;
        do {
            r = static_cast<unsigned_type>(g() - URBG::min());
        } while (r >= limit); // reject if it would introduce bias

        return static_cast<result_type>(p.min() + (r % range));
    }
};
