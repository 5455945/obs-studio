#pragma once
#include <ft2build.h>
#include FT_FREETYPE_H
#include <opencv/cv.h>
#include <opencv/highgui.h>

class CvTextFreeType
{
public:
    // װ���ֿ��ļ�
    CvTextFreeType(const char *freeType);
    virtual ~CvTextFreeType();
    
    //��ȡ���塣Ŀǰ��Щ�����в�֧�֡�
    //param font        ��������, Ŀǰ��֧��
    //param size        �����С/�հױ���/�������/��ת�Ƕ�
    //param underline   �»���
    //param diaphaneity ͸����
    void getFont(int *type, CvScalar *size = nullptr, bool *underline = nullptr, float *diaphaneity = nullptr);
    
    // �������塣Ŀǰ��Щ�����в�֧�֡�
    //param font        ��������, Ŀǰ��֧��
    //param size        �����С/�հױ���/�������/��ת�Ƕ�
    //param underline   �»���
    //param diaphaneity ͸����
    void setFont(int *type, CvScalar *size = nullptr, bool *underline = nullptr, float *diaphaneity = nullptr);
    
    // �ָ�ԭʼ���������á�
    void restoreFont();
    
    // �������(��ɫĬ��Ϊ��ɫ)����������������ַ���ֹͣ��
    //param img  �����Ӱ��
    //param text �ı�����
    //param pos  �ı�λ��
    //return ���سɹ�������ַ����ȣ�ʧ�ܷ���-1��
    int putText(cv::Mat &frame, const char *text, CvPoint pos);
    
    // �������(��ɫĬ��Ϊ��ɫ)����������������ַ���ֹͣ��
    //param img  �����Ӱ��
    //param text �ı�����
    //param pos  �ı�λ��
    //return ���سɹ�������ַ����ȣ�ʧ�ܷ���-1��
    int putText(cv::Mat &frame, const wchar_t *text, CvPoint pos);
    
    // ������֡���������������ַ���ֹͣ��
    //param img   �����Ӱ��
    //param text  �ı�����
    //param pos   �ı�λ��
    //param color �ı���ɫ
    //return ���سɹ�������ַ����ȣ�ʧ�ܷ���-1��
    int putText(cv::Mat &frame, const char *text, CvPoint pos, CvScalar color);
    
    // ������֡���������������ַ���ֹͣ��
    //param img   �����Ӱ��
    //param text  �ı�����
    //param pos   �ı�λ��
    //param color �ı���ɫ
    //return ���سɹ�������ַ����ȣ�ʧ�ܷ���-1��
    int putText(cv::Mat &frame, const wchar_t *text, CvPoint pos, CvScalar color);
    
private:
    // �����ǰ�ַ�, ����m_posλ��
    void putWChar(cv::Mat &frame, wchar_t wc, CvPoint &pos, CvScalar color);
    
private:
    FT_Library  m_library;   // �ֿ�
    FT_Face     m_face;      // ����
    // Ĭ�ϵ������������
    int         m_fontType;
    CvScalar    m_fontSize;
    bool        m_fontUnderline;
    float       m_fontDiaphaneity;
};
