package com.test;

import org.springframework.stereotype.Controller;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.servlet.ModelAndView;

import com.test.mapper.BookMapper;
import com.test.dao.Book;
 
@Controller
public class HelloSpringController {
	private BookMapper bookDao;
    String message = "Welcome to Spring MVC!!!";
 
    @RequestMapping("/hello")
    public ModelAndView showMessage(@RequestParam(value = "name", required = false, defaultValue = "Spring") String name) {
 
    	long bookId = 1000;
    	Book book = bookDao.selectByPrimaryKey(bookId);
    	
    	
    	String bookname = book.getName();
    	//System.out.print(book);
        ModelAndView mv = new ModelAndView("hellospring");//ָ����ͼ
        //����ͼ�������Ҫչʾ��ʹ�õ����ݣ�����ҳ����ʹ��
        mv.addObject("message", message);
        mv.addObject("name", name);
        mv.addObject("bookname", bookname);
        return mv;
    }
}