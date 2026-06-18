#include "Animation.h"

#include <cassert>

Vector3 CalculateValue(
    const std::vector<KeyframeVector3>& keyframes,
    float time
) {
    assert(!keyframes.empty());

    // キーが1つだけ、または先頭より前なら先頭の値を使う。
    if (keyframes.size() == 1 || time <= keyframes[0].time) {
        return keyframes[0].value;
    }

    // time を挟む2つの keyframe を探して線形補間する。
    for (size_t index = 0; index < keyframes.size() - 1; ++index) {
        size_t nextIndex = index + 1;

        if (keyframes[index].time <= time &&
            time <= keyframes[nextIndex].time)
        {
            float t =
                (time - keyframes[index].time) /
                (keyframes[nextIndex].time - keyframes[index].time);

            return Math::Lerp(
                keyframes[index].value,
                keyframes[nextIndex].value,
                t
            );
        }
    }

    // 最後の keyframe より後なら最後の値を使う。
    return keyframes.back().value;
}

Quaternion CalculateValue(
    const std::vector<KeyframeQuaternion>& keyframes,
    float time
) {
    assert(!keyframes.empty());

    if (keyframes.size() == 1 || time <= keyframes[0].time) {
        return keyframes[0].value;
    }

    // Quaternion は回転補間なので Slerp を使う。
    for (size_t index = 0; index < keyframes.size() - 1; ++index) {
        size_t nextIndex = index + 1;

        if (keyframes[index].time <= time &&
            time <= keyframes[nextIndex].time)
        {
            float t =
                (time - keyframes[index].time) /
                (keyframes[nextIndex].time - keyframes[index].time);

            return Math::Slerp(
                keyframes[index].value,
                keyframes[nextIndex].value,
                t
            );
        }
    }

    return keyframes.back().value;
}