#pragma once

#include <string>
#include <vector>
#include <memory>

#include <bstorm/type.hpp>

namespace bstorm {
  struct MqoVec2 {
    float x;
    float y;
  };

  struct MqoVec3 {
    float x;
    float y;
    float z;
  };

  struct MqoColor4 {
    float r;
    float g;
    float b;
    float a;
  };

  // ���C�g
  struct MqoLight {
    MqoVec3 dir = { 0.408, 0.408, 0.816 };
    MqoColor4 color = MqoColor4{ 1.0f, 1.0f, 1.0f, 1.0f };
  };

  // ������̃V�[��
  struct MqoScene {
    MqoVec3 pos = MqoVec3{ 0, 0, 1500 };
    MqoVec3 lookat = MqoVec3{ 0, 0, 0 };
    float head = -0.5236;
    float pich = 0.5236;
    float bank = 0;
    int ortho = 0;
    float zoom2 = 5.0000;
    MqoColor4 amb = MqoColor4{ 0.250, 0.250, 0.250, 1.0 };
    float frontclip = 225.00002;
    float backclip = 45000;
    std::vector<MqoLight> dirlights;
  };

  // �ގ�
  struct MqoMaterial {
    // �ގ���
    std::wstring name;
    // �V�F�[�_�[���
    int shader = 3;
    // ���_�F�̗L��
    bool vcol = false;
    // ���ʕ\�����ǂ���
    bool dbls = false;
    // �F
    MqoColor4 col = MqoColor4{ 1, 1, 1, 1 };
    // �g�U���̖��邳
    float dif = 0.8000;
    // �����̖��邳
    float amb = 0.600;
    // ���Ȕ����̖��邳
    float emi = 0.000;
    float spc = 0.000;
    // ����̋���
    float power = 5.00;
    // ���ʔ���
    float reflect = 1.00;
    // ���ܗ�
    float refract = 1.00;
    // �͗l�}�b�s���O
    std::wstring tex;
    // �����}�b�s���O
    std::wstring aplane;
    // ���ʃ}�b�s���O
    std::wstring bump;
    // �}�b�s���O����
    int proj_type = 0;
    // ���e�ʒu
    MqoVec3 proj_pos = MqoVec3{ 0, 0, 0 };
    MqoVec3 proj_scale = MqoVec3{ 1, 1, 1 };
    MqoVec3 proj_angle = MqoVec3{ 0, 0, 0 };
  };

  // 1�̖�
  struct MqoFace {
    // ���_�C���f�b�N�X
    std::vector<int> vertexIndices;
    // �ގ��C���f�b�N�X
    int materialIndex = 0;
    // ���_UV
    std::vector<MqoVec2> uvs;
    std::vector<MqoColor4> cols;
    std::vector<float> crss;
  };

  // �I�u�W�F�N�g
  struct MqoObject {
    std::wstring name;
    int uid = -1;
    int depth = 0;
    bool folding = false;
    MqoVec3 scale = MqoVec3{ 1, 1, 1 };
    MqoVec3 rotation = MqoVec3{ 0, 0, 0 };
    MqoVec3 translation = MqoVec3{ 0, 0, 0 };
    int patch = 0;
    int patchtri;
    int segment;
    bool visible = true;
    bool locking = false;
    int shading = 1;
    float facet = 59.5;
    MqoColor4 color = MqoColor4{ 0.898, 0.498, 0.698, 1.0 };
    int color_type = 0;
    int mirror = 0;
    int mirror_axis = 1;
    float mirror_dis = 100;
    int lathe;
    int lathe_axis;
    int lathe_seg;
    std::vector<MqoVec3> vertices;
    std::vector<MqoFace> faces;
  };

  struct Mqo {
    std::wstring path;
    float version = 1.0;
    MqoScene scene;
    std::vector<MqoMaterial> materials;
    std::vector<MqoObject> objects;
  };

}