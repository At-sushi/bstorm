#pragma once

namespace bstorm
{
// prefix
constexpr char* DNH_RUNTIME_PREFIX = "r_"; // �����^�C���֐�
constexpr char* DNH_RUNTIME_BUILTIN_PREFIX = "rb_"; // �����^�C�����C�u�����ɏ�����Ă�g�ݍ��݊֐�
#ifdef _DEBUG
constexpr char* DNH_BUILTIN_FUNC_PREFIX = "b_"; // �g�ݍ��݊֐�
#else
constexpr char* DNH_BUILTIN_FUNC_PREFIX = "d_"; // �g�ݍ��݊֐�
#endif
constexpr char* DNH_VAR_PREFIX = "d_"; // �ϐ�
}