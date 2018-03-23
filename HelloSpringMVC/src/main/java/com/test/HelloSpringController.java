package com.test;

import org.springframework.stereotype.Controller;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.servlet.ModelAndView;

import com.test.dao.BookDao;
import com.test.dao.Book;
 
@Controller
public class HelloSpringController {
	private BookDao bookDao;
    String message = "Welcome to Spring MVC!";
 
    @RequestMapping("/hello")
    public ModelAndView showMessage(@RequestParam(value = "name", required = false, defaultValue = "Spring") String name) {
 
    	long bookId =1000;
    	Book book = bookDao.queryById(bookId);

    	String bookname = book.getName();
    	
        ModelAndView mv = new ModelAndView("hellospring");//ָ����ͼ
        //����ͼ�������Ҫչʾ��ʹ�õ����ݣ�����ҳ����ʹ��
        mv.addObject("message", message);
        mv.addObject("name", name);
        mv.addObject("bookname", bookname);
        return mv;
    }
}